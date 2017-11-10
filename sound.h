/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <cinttypes>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <glm/glm.hpp>
#include "dumb3d.h"
#include "parser.h"
#include "Names.h"

class load_error : public std::runtime_error
{
public:
	load_error(std::string const &f);
};

class sound_buffer
{
	ALuint id;
	uint32_t refcount;
	std::chrono::time_point<std::chrono::steady_clock> last_unref;
	int samplerate;

public:
	sound_buffer(std::string &file);
	~sound_buffer();

	int get_samplerate();
	ALuint get_id();
	void ref();
	void unref();
	std::chrono::time_point<std::chrono::steady_clock> unused_since();
};

//m7todo: make constructor/destructor private friend to sound_manager
class sound
{
	bool pos_dirty;
	glm::vec3 last_pos;
	float dt_sum;

public:
	enum mode_t
	{
		global,
		spatial,
		anchored
	};

protected:
	float max_dist;
	mode_t mode;
	glm::vec3 pos;
	int samplerate;

	ALuint id;
	sound();

public:
	float gain_off;
	float gain_mul;
	float pitch_off;
	float pitch_mul;

	virtual ~sound();

	glm::vec3 location(); //get position
	virtual bool is_playing() = 0;

	virtual void play() = 0;
	virtual void stop() = 0;
	virtual void update(float dt);

	sound& set_mode(mode_t);
	sound& dist(float);
	sound& gain(float);
	sound& pitch(float);
	virtual sound& loop(bool loop = true) = 0;

	sound& position(glm::vec3);
	sound& position(Math3D::vector3 const &);
};

class simple_sound : public sound
{
	sound_buffer *buffer;

	bool looping;
	bool playing;

public:
	simple_sound(sound_buffer *buf);
	~simple_sound();

	void play();
	void stop();
	void update(float dt);

	sound& loop(bool loop = true);

	bool is_playing();
};

class complex_sound : public sound
{
	sound_buffer *pre, *buffer, *post;

	enum class state
	{
		premain, // playing pre and continue to main
		prepost, // playing pre and jump to post
		main, //playing main
		post // playing post or idling
	} cs;

	bool shut_by_dist;

public:
	complex_sound(sound_buffer* pre, sound_buffer* main, sound_buffer* post);
	~complex_sound();

	bool is_playing();

	void play();
	void stop();
	void update(float dt);

	sound& loop(bool loop = true);
};

class text_sound : public simple_sound
{
public:
	text_sound(sound_buffer *buf);

	void play();
};

class sound_manager
{
	static bool created;

	ALCdevice *dev;
	ALCcontext *ctx;

	void (*alDeferUpdatesSOFT)() = nullptr;
	void (*alProcessUpdatesSOFT)() = nullptr;
	void (*alcDevicePauseSOFT)(ALCdevice*) = nullptr;
	void (*alcDeviceResumeSOFT)(ALCdevice*) = nullptr;

	const std::chrono::duration<float> gc_time = std::chrono::duration<float>(60.0f);
	std::unordered_map<std::string, sound_buffer*> buffers;
	std::unordered_set<sound*> sounds;

	std::string find_file(std::string);
	sound_buffer* find_buffer(std::string);
	
	sound_buffer* get_buffer(std::string const &);

public:
	glm::vec3 pos, last_pos;
	glm::mat3 rot;

	sound_manager();
	~sound_manager();

	simple_sound* create_sound(std::string const &file);
	text_sound* create_text_sound(std::string const &file);
	complex_sound* create_complex_sound(std::string const &pre, std::string const &main, std::string const &post);
	complex_sound* create_complex_sound(cParser &);
	void destroy_sound(sound**);

	void update(float dt);
	void set_listener(glm::vec3 const &pos, glm::mat3 const &rot);
};

extern std::unique_ptr<sound_manager> sound_man;