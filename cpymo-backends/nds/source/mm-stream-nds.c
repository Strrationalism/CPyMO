#include <stdlib.h>
#include <calico.h>
#include <maxmod9.h>
/* Derived from devkitPro/maxmod v2.1.0 source_calico/mm_stream.c.
   Corresponding license in ../licenses/maxmod-LICENSE.
   Local change: freeze/rebase the stream tick on console sleep/wakeup.
   Hardware channel phase and the original prefilled buffer are preserved. */

#define SOUND_CLOCKS_PER_TICK (64/2)

typedef union mm_buf {
	void* ptr;
	u16*  ptr16;
	u8*   ptr8;
} mm_buf;

static u32 s_mmStreamMailboxSlots[2];

static struct {
	TickTask task;

	Thread* thread;
	Mailbox mbox;

	mm_stream_func cb;

	mm_buf buf;
	mm_buf workbuf;

	unsigned page_cnt;
	unsigned page_len;
	unsigned timer;

	size_t right_offset;

	unsigned write_pos;
	unsigned read_pos;

	u8 fmt_shift;
	mm_stream_formats fmt;
} s_mmStreamState;

static void _mmStreamTickTask(TickTask* t)
{
	s_mmStreamState.page_cnt++;

	if (s_mmStreamState.mbox.pending_slots == 0) {
		mailboxTrySend(&s_mmStreamState.mbox, 1);
	}
}

void _mmDeinterleave8 (unsigned len, const u8* in,  u8* out,  unsigned right_offset);
void _mmDeinterleave16(unsigned len, const u16* in, u16* out, unsigned right_offset);

static unsigned _mmStreamRead(unsigned pos, unsigned len)
{
	mm_buf left_buf = { &s_mmStreamState.buf.ptr8[pos<<s_mmStreamState.fmt_shift] };
	unsigned right_offset = s_mmStreamState.right_offset;
	unsigned fmt_shift = s_mmStreamState.fmt_shift;

	// Write to audio buf directly if mono, otherwise use workbuf for interleaved stereo
	mm_buf cb_buf = left_buf;
	if (right_offset) {
		cb_buf = s_mmStreamState.workbuf;
	}

	// Generate audio data using user callback
	unsigned ret = s_mmStreamState.cb(len, cb_buf.ptr, s_mmStreamState.fmt);
	if (ret == 0 || ret > len) {
		return 0;
	}

	if (right_offset) {
		// Deinterleave the audio data into the final left/right buffers
		if (fmt_shift) {
			_mmDeinterleave16(ret, cb_buf.ptr16, left_buf.ptr16, right_offset);
		} else {
			_mmDeinterleave8 (ret, cb_buf.ptr8,  left_buf.ptr8,  right_offset);
		}

		mm_buf right_buf = { &left_buf.ptr8[right_offset] };
		armDCacheFlush(right_buf.ptr, ret<<fmt_shift);
	}

	armDCacheFlush(left_buf.ptr, ret<<fmt_shift);

	return ret;
}

static void _mmStreamRefill(void)
{
	unsigned total_len = s_mmStreamState.page_len*2;
	unsigned rd_pos = s_mmStreamState.read_pos;
	unsigned wr_pos = s_mmStreamState.write_pos;

	do {
		unsigned len;

		if (rd_pos > wr_pos) {
			len = _mmStreamRead(wr_pos, rd_pos-wr_pos);
		} else {
			len = _mmStreamRead(wr_pos, total_len-wr_pos);
		}

		if (!len) {
			break;
		}

		wr_pos += len;
		if (wr_pos == total_len) {
			wr_pos = 0;
		}

	} while (wr_pos != rd_pos);

	s_mmStreamState.write_pos = wr_pos;
}

MK_NOINLINE static void _mmStreamStart(u32 chn_mask, unsigned task_period)
{
	// Prefill audio buffers
	_mmStreamRefill();

	// Atomically start channel(s) and afterwards the stream tick task
	ArmIrqState st = armIrqLockByPsr();
	soundStart(chn_mask);
	soundSynchronize();
	tickTaskStart(&s_mmStreamState.task, _mmStreamTickTask, task_period, task_period);
	armIrqUnlockByPsr(st);
}

MK_NOINLINE static void _mmStreamStop(u32 chn_mask)
{
	// Stop counter tick task
	tickTaskStop(&s_mmStreamState.task);

	// Stop channels and return them to maxmod's ARM7 engine
	soundStop(chn_mask);
	soundSynchronize();
	mmUnlockChannels(chn_mask);
}

static int _mmStreamThread(void* arg)
{
	u32 chn_mask = s_mmStreamState.right_offset ? 5 : 1;
	unsigned task_period = (unsigned)arg;

	_mmStreamStart(chn_mask, task_period);

	while (mailboxRecv(&s_mmStreamState.mbox) != 0) {
		mmStreamUpdate();
	}

	_mmStreamStop(chn_mask);

	return 0;
}

void mmStreamOpen(const mm_stream* stream)
{
	// Close stream if already open
	if (s_mmStreamState.task.fn || s_mmStreamState.thread) {
		mmStreamClose();
	}

	// Calculate page length (half buffer)
	unsigned page_len = (stream->buffer_length + 1) / 2;
	page_len = (page_len + SOUND_CLOCKS_PER_TICK - 1) &~ (SOUND_CLOCKS_PER_TICK - 1);

	// Calculate timing constants
	unsigned sound_timer = soundTimerFromHz(stream->sampling_rate);
	unsigned task_period = (page_len/SOUND_CLOCKS_PER_TICK) * sound_timer;

	// Other misc useful values
	unsigned fmt_shift = stream->format >= MM_STREAM_16BIT_MONO;
	unsigned chn_shift = stream->format & 1;

	// Calculate total buffer size
	unsigned buf_len_words = page_len >> (1-fmt_shift);      // only one channel
	size_t buf_sz = page_len << (1 + fmt_shift + chn_shift); // if stereo: including both channels

	// Allocate main audio buffer
	s_mmStreamState.buf.ptr = aligned_alloc(ARM_CACHE_LINE_SZ, buf_sz);
	if (!s_mmStreamState.buf.ptr) {
		return;
	}

	// Allocate separate interleaved work buffer for stereo streams
	if (chn_shift) {
		s_mmStreamState.workbuf.ptr = malloc(buf_sz);
		s_mmStreamState.right_offset = buf_sz/2;

		if (!s_mmStreamState.workbuf.ptr) {
			free(s_mmStreamState.buf.ptr);
			return;
		}
	} else {
		s_mmStreamState.workbuf.ptr = NULL;
		s_mmStreamState.right_offset = 0;
	}

	// Allocate and prepare stream thread if needed
	if (stream->minus_thread_prio <= 0) {
		// Calculate needed size for thread
		size_t thread_sz = stream->thread_stack_size ? stream->thread_stack_size : 8*1024;
		thread_sz = (sizeof(Thread) + threadGetLocalStorageSize() + thread_sz + 7) &~ 7;

		// Allocate thread memory
		s_mmStreamState.thread = malloc(thread_sz);
		if (!s_mmStreamState.thread) {
			free(s_mmStreamState.workbuf.ptr);
			free(s_mmStreamState.buf.ptr);
			return;
		}

		// Prepare thread
		threadPrepare(s_mmStreamState.thread, _mmStreamThread, (void*)task_period,
			(u8*)s_mmStreamState.thread + thread_sz,
			stream->minus_thread_prio < 0 ? (-stream->minus_thread_prio) : (MAIN_THREAD_PRIO+1));
		threadAttachLocalStorage(s_mmStreamState.thread, NULL);
	}

	// Initialize stream state struct
	mailboxPrepare(&s_mmStreamState.mbox, s_mmStreamMailboxSlots, 2);
	s_mmStreamState.cb = stream->callback;
	s_mmStreamState.page_cnt = 0;
	s_mmStreamState.page_len = page_len;
	s_mmStreamState.timer = sound_timer;
	s_mmStreamState.read_pos = 0;
	s_mmStreamState.write_pos = 0;
	s_mmStreamState.fmt_shift = fmt_shift;
	s_mmStreamState.fmt = stream->format;

	// Reserve channels from maxmod's ARM7 engine
	u32 chn_mask = chn_shift ? 5 : 1;
	mmLockChannels(chn_mask);

	// Prepare (left) channel
	SoundFmt sound_fmt = fmt_shift ? SoundFmt_Pcm16 : SoundFmt_Pcm8;
	soundPreparePcm(0, 0x7f0, chn_shift ? 0 : 0x40, sound_timer, SoundMode_Repeat, sound_fmt,
		s_mmStreamState.buf.ptr, 0, buf_len_words);

	// Prepare right channel if needed
	if (chn_shift) {
		soundPreparePcm(2, 0x7f0, 0x7f, sound_timer, SoundMode_Repeat, sound_fmt,
			&s_mmStreamState.buf.ptr8[s_mmStreamState.right_offset], 0, buf_len_words);
	}

	if (stream->minus_thread_prio > 0) {
		// Start the stream directly
		_mmStreamStart(chn_mask, task_period);
	} else {
		// Start the stream thread
		threadStart(s_mmStreamState.thread);
	}
}

static mm_word _mmStreamGetPosition(mm_word mask)
{
	// Atomically read current page + tick position
	ArmIrqState st = armIrqLockByPsr();
	unsigned cur_page = s_mmStreamState.page_cnt;
	unsigned ticks = (unsigned)tickGetCount() - (s_mmStreamState.task.target - s_mmStreamState.task.period);
	armIrqUnlockByPsr(st);

	// XX: For safety's sake, account for not-yet-triggered page switch events
	while (ticks >= s_mmStreamState.task.period) {
		cur_page ++;
		ticks -= s_mmStreamState.task.period;
	}

	// Combine page counter + fine grained sample counter
	mm_word coarse = (cur_page & mask) * s_mmStreamState.page_len;
	mm_word fine = ticks*SOUND_CLOCKS_PER_TICK/s_mmStreamState.timer;

	return coarse + fine;
}

volatile unsigned stream_diag[8]={0x53545244};
static unsigned diag_last_pos;static bool diag_primed;
void nds_stream_diag_reset(void){for(unsigned i=1;i<8;i++)stream_diag[i]=0;diag_last_pos=mmStreamGetPosition();diag_primed=true;}
/* R15: distinguish no newly consumed samples from initial/full-wrap prefill. */
volatile unsigned refill_diag[16]={0x52464c31};
static bool refill_guard;
void nds_stream_guard_policy(bool enabled){refill_guard=enabled;refill_diag[1]=enabled;}
void mmStreamUpdate(void)
{
	if (!s_mmStreamState.task.fn) {
		return;
	}

	if (s_mmStreamState.thread && threadGetSelf() != s_mmStreamState.thread) {
		return;
	}

	unsigned absolute=mmStreamGetPosition();unsigned elapsed_samples=diag_primed?absolute-diag_last_pos:0;if(diag_primed){unsigned elapsed=absolute-diag_last_pos;if(elapsed>stream_diag[2])stream_diag[2]=elapsed;if(elapsed>=s_mmStreamState.page_len*2)stream_diag[3]++;}diag_last_pos=absolute;diag_primed=true;stream_diag[1]++;
 s_mmStreamState.read_pos = _mmStreamGetPosition(1) &~ 3; // only allow 4-sample increments
	bool equal=s_mmStreamState.read_pos==s_mmStreamState.write_pos;
 if(equal){refill_diag[2]++;if(elapsed_samples<s_mmStreamState.page_len*2){refill_diag[3]++;if(refill_guard){refill_diag[4]++;return;}}else refill_diag[5]++;}
 refill_diag[6]++;
 u64 diag_begin=tickGetCount();_mmStreamRefill();unsigned diag_us=(tickGetCount()-diag_begin)*1000000/TICK_FREQ;if(diag_us>stream_diag[4])stream_diag[4]=diag_us;
}

void mmStreamClose(void)
{
	// Check for and stop the stream thread
	if (s_mmStreamState.thread) {
		mailboxTrySend(&s_mmStreamState.mbox, 0);
		threadJoin(s_mmStreamState.thread);
		free(s_mmStreamState.thread);
	}
	// Otherwise: check for and stop manually started streams
	else if (s_mmStreamState.task.fn) {
		u32 chn_mask = s_mmStreamState.right_offset ? 5 : 1;
		_mmStreamStop(chn_mask);
	}
	// Otherwise: no stream is active
	else {
		return;
	}

	// Free buffers
	free(s_mmStreamState.workbuf.ptr);
	free(s_mmStreamState.buf.ptr);

	// Clear all state
	armFillMem32(&s_mmStreamState, 0, sizeof(s_mmStreamState));
}

mm_word mmStreamGetPosition(void)
{
	if (!s_mmStreamState.task.fn) {
		return 0;
	}

	return _mmStreamGetPosition(~0U);
}

static unsigned nds_stream_remaining;
static bool nds_stream_suspended;
void nds_mm_stream_suspend(void){
 if(!s_mmStreamState.task.fn||nds_stream_suspended)return;
 ArmIrqState st=armIrqLockByPsr();
 unsigned now=(unsigned)tickGetCount();nds_stream_remaining=s_mmStreamState.task.target-now;
 if((s32)nds_stream_remaining<1)nds_stream_remaining=1;
 tickTaskStop(&s_mmStreamState.task);nds_stream_suspended=true;
 armIrqUnlockByPsr(st);
}
void nds_mm_stream_resume(void){
 if(!nds_stream_suspended)return;
 ArmIrqState st=armIrqLockByPsr();
 tickTaskStart(&s_mmStreamState.task,_mmStreamTickTask,nds_stream_remaining,s_mmStreamState.task.period);
 nds_stream_suspended=false;armIrqUnlockByPsr(st);
}
