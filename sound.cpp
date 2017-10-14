/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "stdafx.h"
#include "McZapkie/mctools.h"
#include "Globals.h"
#include "sound.h"
#include "Logs.h"
#include <sndfile.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

load_error::load_error(std::string const &f) : std::runtime_error("sound: cannot find " + f)
{

}

sound_manager::sound_manager()
{
	if (created)
		throw std::runtime_error("sound_manager can be instantinated only once");
	created = true;

	dev = alcOpenDevice(0);
	if (!dev)
		throw std::runtime_error("sound: cannot open device");

	ALCint attr[3] = { ALC_MONO_SOURCES, 20000, 0 };
	// we're requesting horrible max amount of sources here
	// because we create AL source object for each sound object,
	// even if not active currently. considier creating AL source
	// object only when source is played and destroying it afterwards

	ctx = alcCreateContext(dev, attr);
	if (!ctx)
		throw std::runtime_error("sound: cannot create context");

	if (!alcMakeContextCurrent(ctx))
		throw std::runtime_error("sound: cannot select context");

	if (alIsExtensionPresent("AL_SOFT_deferred_updates"))
	{
		alDeferUpdatesSOFT = (void(*)())alGetProcAddress("alDeferUpdatesSOFT");
		alProcessUpdatesSOFT = (void(*)())alGetProcAddress("alProcessUpdatesSOFT");
	}
	if (!alDeferUpdatesSOFT || !alProcessUpdatesSOFT)
		WriteLog("sound: warning: extension AL_SOFT_deferred_updates not found");

	if (alcIsExtensionPresent(dev, "ALC_SOFT_pause_device"))
	{
		alcDevicePauseSOFT = (void(*)(ALCdevice*))alcGetProcAddress(dev, "alcDevicePauseSOFT");
		alcDeviceResumeSOFT = (void(*)(ALCdevice*))alcGetProcAddress(dev, "alcDeviceResumeSOFT");
	}
	if (!alcDevicePauseSOFT || !alcDeviceResumeSOFT)
		WriteLog("sound: warning: extension ALC_SOFT_pause_device not found");

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	alGetError();
}

sound_manager::~sound_manager()
{
	alcMakeContextCurrent(0);
	alcDestroyContext(ctx);
	alcCloseDevice(dev);

	created = false;
}

sound_buffer* sound_manager::find_buffer(std::string name)
{
	size_t pos = name.rfind('.');
	if (pos != name.npos)
		name.erase(pos);

	std::replace(name.begin(), name.end(), '\\', '/');

	auto search = buffers.find(name);
	if (search != buffers.end())
		return search->second;
	else
		return nullptr;
}

std::string sound_manager::find_file(std::string name)
{
	if (FileExists(name))
		return name;

    size_t pos = name.rfind('.');
    if (pos != name.npos)  
        name.erase(pos);

	std::vector<std::string> exts { ".wav", ".WAV", ".flac", ".ogg" };
	for (auto const &ext : exts)
		if (FileExists(name + ext))
			return name + ext;

	return "";
}

sound_buffer* sound_manager::get_buffer(std::string const &name)
{
	sound_buffer* buf = find_buffer(Global::asCurrentDynamicPath + name);
	if (buf)
		return buf;

	buf = find_buffer("sounds/" + name);
	if (buf)
		return buf;

	std::string file = find_file(Global::asCurrentDynamicPath + name);
	if (!file.size())
		file = find_file("sounds/" + name);
	if (!file.size())
		file = find_file(name);

	if (!file.size())
		throw load_error(name);

	std::replace(file.begin(), file.end(), '\\', '/');

	buf = new sound_buffer(file);
	
    size_t pos = file.rfind('.');
    if (pos != file.npos)  
        file.erase(pos);
	buffers.emplace(file, buf);

	return buf;
}

simple_sound* sound_manager::create_sound(std::string const &file)
{
	try
	{
		WriteLog("creating source with " + file);
		simple_sound *s = new simple_sound(get_buffer(file));
		sounds.emplace(s);
		return s;
	}
	catch (load_error& e)
	{
		WriteLog(e.what());
	}
	return nullptr;
}

text_sound* sound_manager::create_text_sound(std::string const &file)
{
	try
	{
		WriteLog("creating source with " + file);
		text_sound *s = new text_sound(get_buffer(file));
		sounds.emplace(s);
		return s;
	}
	catch (load_error& e)
	{
		WriteLog(e.what());
	}
	return nullptr;
}

complex_sound* sound_manager::create_complex_sound(std::string const &pre, std::string const &main, std::string const &post)
{
	try
	{
		WriteLog("creating source with " + pre + ", " + main + ", " + post);
		complex_sound *s = new complex_sound(get_buffer(pre), get_buffer(main), get_buffer(post));
		sounds.emplace(s);
		return s;
	}
	catch (load_error& e)
	{
		WriteLog(e.what());
	}
	return nullptr;
}

complex_sound* sound_manager::create_complex_sound(cParser &parser)
{
	std::string pre, main, post;
	double attenuation;

	parser.getTokens(3, true, "\n\t ;,"); // samples separated with commas
	parser >> pre >> main >> post;
	parser.getTokens(1, false);
	parser >> attenuation;

	complex_sound* s = create_complex_sound(pre, main, post);
	if (s)
		s->dist(attenuation);
	return s;
}

void sound_manager::destroy_sound(sound **s)
{
	if (*s != nullptr)
	{
		sounds.erase(*s);
		delete *s;
		*s = nullptr;
	}
}

void sound_manager::update(float dt)
{
	ALenum err = alGetError();
	if (err != AL_NO_ERROR)
	{
		std::string errname;
		if (err == AL_INVALID_NAME)
			errname = "AL_INVALID_NAME";
		else if (err == AL_INVALID_ENUM)
			errname = "AL_INVALID_ENUM";
		else if (err == AL_INVALID_VALUE)
			errname = "AL_INVALID_VALUE";
		else if (err == AL_INVALID_OPERATION)
			errname = "AL_INVALID_OPERATION";
		else if (err == AL_OUT_OF_MEMORY)
			errname = "AL_OUT_OF_MEMORY";
		else
			errname = "?";
		
		throw std::runtime_error("sound: al error: " + errname);
	}

	if (dt > 0.0f)
	{
		if (alcDeviceResumeSOFT)
			alcDeviceResumeSOFT(dev);

		auto now = std::chrono::steady_clock::now();
		auto it = buffers.begin();
		while (it != buffers.end())
		{
			if (now - it->second->unused_since() > gc_time)
			{
				delete it->second;
				it = buffers.erase(it);
			}
			else
				it++;
		}

		glm::vec3 velocity = (pos - last_pos) / dt;
		alListenerfv(AL_VELOCITY, glm::value_ptr(velocity));
		last_pos = pos;

		for (auto &s : sounds)
			s->update(dt);
	}
	else
	{
		if (alcDevicePauseSOFT)
			alcDevicePauseSOFT(dev);
	}


	if (alProcessUpdatesSOFT)
	{
		alProcessUpdatesSOFT();
		alDeferUpdatesSOFT();
	}
}

void sound_manager::set_listener(glm::vec3 const &p, glm::mat3 const &r)
{
	pos = p;
	rot = r;
	glm::vec3 at = glm::vec3(0.0, 0.0, -1.0) * r;
	glm::vec3 up = glm::vec3(0.0, 1.0, 0.0) * r;

	alListenerfv(AL_POSITION, glm::value_ptr(pos));
	glm::vec3 ori[] = { at, up };
	alListenerfv(AL_ORIENTATION, (ALfloat*)ori);
}

sound::sound()
{
	id = 0;
	alGenSources(1, &id);
	if (!id)
		throw std::runtime_error("sound: cannot generate source");
	dist(5.0f * 3.82f);
	set_mode(global);
	gain_off = 0.0f;
	gain_mul = 1.0f;
	pitch_off = 0.0f;
	pitch_mul = 1.0f;
	dt_sum = 0.0f;
}

sound& sound::set_mode(mode_t m)
{
	mode = m;

	if (mode == global || mode == anchored)
		alSourcei(id, AL_SOURCE_RELATIVE, AL_TRUE);
	else if (mode == spatial)
		alSourcei(id, AL_SOURCE_RELATIVE, AL_FALSE);

	return *this;
}

simple_sound::simple_sound(sound_buffer *buf) : sound::sound()
{
	looping = false;
	playing = false;
	buffer = buf;
	alSourcei(id, AL_BUFFER, buffer->get_id());
	buffer->ref();
	samplerate = buffer->get_samplerate();
}

sound::~sound()
{
	alDeleteSources(1, &id);
}

simple_sound::~simple_sound()
{
	buffer->unref();
}

sound& sound::position(glm::vec3 p)
{
	if (mode == global)
	{
		set_mode(spatial);
		last_pos = p;
	}

	if (p != pos || mode == anchored)
	{
		pos = p;
		pos_dirty = true;

		if (mode == anchored)
			p = (p - sound_man->pos) * glm::inverse(sound_man->rot);

		alSourcefv(id, AL_POSITION, glm::value_ptr(p));
	}

	return *this;
}

sound& sound::position(Math3D::vector3 const &pos)
{
	position((glm::vec3)glm::make_vec3(&pos.x));
	return *this;
}

sound& sound::dist(float dist)
{
	max_dist = dist * 2.0f;
	float half_dist = dist / 3.82f;
	alSourcef(id, AL_REFERENCE_DISTANCE, half_dist / 1.32f);
	alSourcef(id, AL_ROLLOFF_FACTOR, 3.0f);
	alSourcef(id, AL_MAX_DISTANCE, max_dist);
	return *this;
}

void simple_sound::play()
{
	if (playing || (mode != global && glm::distance(pos, sound_man->pos) > max_dist))
		return;

	alSourcePlay(id);
	playing = true;
}

void simple_sound::stop()
{
	if (!playing)
		return;

	alSourceStop(id);
	playing = false;
}

void sound::update(float dt)
{
	if (mode == spatial)
		dt_sum += dt;
	if (mode == spatial && pos_dirty)
	{
		glm::vec3 velocity = (pos - last_pos) / dt_sum; // m/s
		alSourcefv(id, AL_VELOCITY, glm::value_ptr(velocity));
		last_pos = pos;
		pos_dirty = false;
		dt_sum = 0.0f;
	}
}

void simple_sound::update(float dt)
{
	sound::update(dt);

	if (playing)
	{
	    ALint v;
	    alGetSourcei(id, AL_SOURCE_STATE, &v);
		if (v != AL_PLAYING)
			playing = false;
		else if (mode != global && glm::distance(pos, sound_man->pos) > max_dist)
			stop();
	}
}

sound& sound::gain(float gain)
{
	gain = gain * gain_mul + gain_off;
	if (Global::soundgainmode == Global::scaled)
		gain /= 1.75f;
	if (Global::soundgainmode == Global::compat)
		gain = std::pow(10.0f, ((-50.0f + 50.0f * gain) / 20.0f));
	gain = std::min(std::max(0.0f, gain), 2.0f);
	alSourcef(id, AL_GAIN, gain);
	return *this;
}

sound& sound::pitch(float pitch)
{
	pitch = pitch * pitch_mul + pitch_off;
	if (Global::soundpitchmode == Global::compat)
		pitch *= 22050.0f / (float)samplerate;
	pitch = std::min(std::max(0.05f, pitch), 20.0f);
	alSourcef(id, AL_PITCH, pitch);
	return *this;
}

sound& simple_sound::loop(bool loop)
{
	if (loop != looping)
	{
		alSourcei(id, AL_LOOPING, (ALint)loop);
		looping = loop;
	}
	return *this;
}

bool simple_sound::is_playing()
{
	return playing;
}

//m7todo: implement text_sound
text_sound::text_sound(sound_buffer *buf) : simple_sound::simple_sound(buf)
{

}

void text_sound::play()
{
	simple_sound::play();
}

complex_sound::complex_sound(sound_buffer* pre, sound_buffer* main, sound_buffer* post) : sound::sound()
{
	this->pre = pre;
	this->buffer = main;
	this->post = post;

	pre->ref();
	buffer->ref();
	post->ref();

	samplerate = buffer->get_samplerate();
	shut_by_dist = false;

	cs = state::post;
}

complex_sound::~complex_sound()
{
	pre->unref();
	buffer->unref();
	post->unref();
}

void complex_sound::play()
{
	if (cs != state::post)
		return;

    if (mode != global && glm::distance(pos, sound_man->pos) > max_dist)
        return;

	alSourceRewind(id);

	if (shut_by_dist)
	{
		shut_by_dist = false;

		alSourcei(id, AL_LOOPING, AL_TRUE);
		alSourcei(id, AL_BUFFER, 0);
		alSourcei(id, AL_BUFFER, buffer->get_id());
		alSourcePlay(id);

		cs = state::main;
	}
	else
	{
		alSourcei(id, AL_LOOPING, AL_FALSE);
		alSourcei(id, AL_BUFFER, 0);
		ALuint buffers[] = { pre->get_id(), buffer->get_id() };
		alSourceQueueBuffers(id, 2, buffers);
		alSourcePlay(id);

		cs = state::premain;
	}
}

void complex_sound::stop()
{
	if (shut_by_dist)
	{
		alSourceRewind(id);

		cs = state::post;
	}
	else if (cs == state::main || (Global::soundstopmode == Global::playstop && cs == state::premain))
	{
		alSourceRewind(id);

		alSourcei(id, AL_LOOPING, AL_FALSE);
		alSourcei(id, AL_BUFFER, 0);
		alSourcei(id, AL_BUFFER, post->get_id());

		alSourcePlay(id);

		cs = state::post;
	}
	else if (cs == state::premain)
	{
		if (Global::soundstopmode == Global::queue)
			cs = state::prepost;
		else if (Global::soundstopmode == Global::stop)
		{
			alSourceRewind(id);
			cs = state::post;
		}
	}
}

void complex_sound::update(float dt)
{
	sound::update(dt);

	if (cs == state::premain || cs == state::prepost)
	{
		ALint processed;
		alGetSourcei(id, AL_BUFFERS_PROCESSED, &processed);
		if (processed > 0) // already processed pre
		{
			if (cs == state::premain)
			{
				cs = state::main;

				ALuint pre_id = pre->get_id();
				alSourceUnqueueBuffers(id, 1, &pre_id);
				alSourcei(id, AL_LOOPING, AL_TRUE);
				if (processed > 1) // underrun occured
					alSourcePlay(id);

			}
			else if (cs == state::prepost)
			{
				cs = state::main;
				stop();
			}
		}
	}

	if (cs == state::main)
		if (mode != global && glm::distance(pos, sound_man->pos) > max_dist)
		{
			shut_by_dist = true;
			stop();
		}
}

sound& complex_sound::loop(bool loop)
{
	throw std::runtime_error("cannot loop complex_sound");
	return *this;
}

bool complex_sound::is_playing()
{
	return cs != state::post; // almost accurate
}

int sound_buffer::get_samplerate()
{
	return samplerate;
}

sound_buffer::sound_buffer(std::string &file)
{
	WriteLog("creating sound buffer from " + file);

	SF_INFO si;
	si.format = 0;

	SNDFILE *sf = sf_open(file.c_str(), SFM_READ, &si);
	if (sf == nullptr)
		throw std::runtime_error("sound: sf_open failed");

	int16_t *fbuf = new int16_t[si.frames * si.channels];
	if (sf_readf_short(sf, fbuf, si.frames) != si.frames)
		throw std::runtime_error("sound: incomplete file");

	sf_close(sf);

	samplerate = si.samplerate;

	int16_t *buf = nullptr;
	if (si.channels == 1)
		buf = fbuf;
	else
	{
		WriteLog("sound: warning: mixing multichannel file to mono");
		buf = new int16_t[si.frames];
		for (size_t i = 0; i < si.frames; i++)
		{
			int32_t accum = 0;
			for (size_t j = 0; j < si.channels; j++)
				accum += fbuf[i * si.channels + j];
			buf[i] = accum / si.channels;
		}
	}

	id = 0;
	alGenBuffers(1, &id);
	if (!id)
		throw std::runtime_error("sound: cannot generate buffer");

	alBufferData(id, AL_FORMAT_MONO16, buf, si.frames * 2, si.samplerate);

	if (si.channels != 1)
		delete[] buf;
	delete[] fbuf;
}

sound_buffer::~sound_buffer()
{
	alDeleteBuffers(1, &id);
}

void sound_buffer::ref()
{
	refcount++;
}

void sound_buffer::unref()
{
	refcount--;
	last_unref = std::chrono::steady_clock::now();
}

ALuint sound_buffer::get_id()
{
	return id;
}

std::chrono::time_point<std::chrono::steady_clock> sound_buffer::unused_since()
{
	if (refcount > 0)
		return std::chrono::time_point<std::chrono::steady_clock>::max();

	return last_unref;
}

bool sound_manager::created = false;
