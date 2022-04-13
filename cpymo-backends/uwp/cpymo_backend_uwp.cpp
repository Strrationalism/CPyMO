#include <string>
#include <assert.h>
#include <SDL.h>
using namespace Windows::Storage;

template <typename T>
class maybe final
{
private:
	bool is_just_;
	T value_;

public:
	maybe() : is_just_{ false } {};
	maybe(T&& v) : is_just_{ true }, value_{ std::move(v) } {};
	bool is_just() const { return is_just_; }
	T get_move_out() {
		assert(is_just_);
		is_just_ = false;
		return std::move(value_);
	}

	T& get() {
		assert(is_just_);
		return value_;
	}
};

template <typename T>
maybe<T> wait_async(Windows::Foundation::IAsyncOperation<T> ^async) {
	while(async->Status != Windows::Foundation::AsyncStatus::Completed) {
		SDL_Delay(1);
		if (async->Status == Windows::Foundation::AsyncStatus::Error) {
			return maybe<T>{};
		}
	}
	return maybe<T>{ async->GetResults() };
}

void enum_games() {
	auto x = wait_async(wait_async(KnownFolders::DocumentsLibrary->GetFolderAsync("pymogames")).get()->GetFileAsync("default.ttf")).get();
	std::wstring font_path = x->Path->Data();
}

