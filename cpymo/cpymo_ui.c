#include "cpymo_ui.h"
#include "cpymo_engine.h"
#include <stdlib.h>
#include <assert.h>

error_t cpymo_ui_enter(void ** out_uidata, cpymo_engine *e, size_t ui_data_size, cpymo_ui_updater u, cpymo_ui_drawer d, cpymo_ui_deleter deleter)
{
	if (e->ui != NULL) cpymo_ui_exit(e);

	cpymo_ui *ui = (cpymo_ui*)malloc(sizeof(cpymo_ui) + ui_data_size);
	if (ui == NULL) return CPYMO_ERR_OUT_OF_MEM;

	ui->update = u;
	ui->draw = d;
	ui->deleter = deleter;

	assert(*out_uidata == NULL);
	*out_uidata = ui + 1;

	e->ui = ui;

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

void cpymo_ui_exit(cpymo_engine *e)
{
	assert(e->ui);
	e->ui->deleter(e, e->ui + 1);
	free(e->ui);
	e->ui = NULL;

	cpymo_engine_request_redraw(e);
}

void *cpymo_ui_data(cpymo_engine *e)
{
	return e->ui + 1;
}

error_t cpymo_ui_update(cpymo_engine *e, float dt)
{
	return e->ui->update(e, e->ui + 1, dt);
}

void cpymo_ui_draw(const cpymo_engine *e)
{
	e->ui->draw(e, e->ui + 1);
}

bool cpymo_ui_enabled(const cpymo_engine *e)
{
	return e->ui != NULL;
}
