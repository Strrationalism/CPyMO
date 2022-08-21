#include "cpymo_prelude.h"
#include <string>
#include <cvt/wstring>
#include <codecvt>
#include <assert.h>
#include <SDL.h>
#include <cpymo_game_selector.h>

using namespace Windows::Storage;

template <typename T>
class maybe final {
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

std::string w2c(Platform::String ^s) {
	stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
	return convert.to_bytes(s->Data());
}


cpymo_game_selector_item *get_game_list(const char *game_selector_dir)
{
	cpymo_game_selector_item *head = NULL, *tail = NULL;
	auto local_folder = ApplicationData::Current->LocalFolder;

	auto sub_folders = wait_async(local_folder->GetFoldersAsync());
	if (sub_folders.is_just()) {
		for (unsigned i = 0; i < sub_folders.get()->Size; ++i) {
			std::string path = w2c(sub_folders.get()->GetAt(i)->Path);
			path = path;

			cpymo_game_selector_item *cur = (cpymo_game_selector_item *)malloc(sizeof(cpymo_game_selector_item));
			if (cur == NULL) continue;

			memset(cur, 0, sizeof(cpymo_game_selector_item));

			char *path_c = (char *)malloc(path.size() + 1);
			if (path_c == NULL) {
				free(cur); 
				continue;
			}

			strcpy(path_c, path.c_str());
			cur->gamedir = path_c;
			cur->next = NULL;

			if (head == NULL) head = cur;
			if (tail != NULL) tail->next = cur;
			tail = cur;
		}

			
	}
	

	return head;
}
