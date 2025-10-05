#pragma once
#include<SDL.h>
#include<memory>
#include<SDL_mixer.h>
#include<Logger/Logger.h>

namespace UTIL {
	//定义一个结构体，重构其函数调用运算符()；这里相当于把一个结构体当作函数，用于销毁
	struct SDL_Destoryer
	{
		void operator()(SDL_Window* window) const;
		void operator()(SDL_GameController* controller) const;
		void operator()(Mix_Chunk* chunk) const;
		void operator()(Mix_Music* music) const;
		void operator()(SDL_Cursor* cursor) const;
	};
}

//std::shared_ptr 是一种智能指针，它会自动管理内存。当最后一个指向 SDL_GameController 对象的 shared_ptr 被销毁时，它会自动释放内存。这可以有效防止内存泄漏。
typedef std::shared_ptr<SDL_GameController> Controller;
static Controller make_shared_controller(SDL_GameController* controller);

typedef std::shared_ptr<SDL_Cursor> Cursor;
static Cursor make_shared_cursor(SDL_Cursor* cursor);

typedef std::unique_ptr<SDL_Window, UTIL::SDL_Destoryer> WindowPtr;
//定义了一个名为WindowPtr的类型，它是std::unique_ptr的特化版本：
//管理的资源类型是SDL_Window
//使用自定义的UTIL::SDL_Destoryer作为删除器
//这样定义后，WindowPtr会自动管理音频资源的生命周期，当智能指针超出作用域时，会自动调用SDL_Destoryer来释放资源，避免内存泄漏

typedef std::unique_ptr<Mix_Chunk, UTIL::SDL_Destoryer> SoundPtr;
typedef std::unique_ptr<Mix_Music, UTIL::SDL_Destoryer> MusicPtr;