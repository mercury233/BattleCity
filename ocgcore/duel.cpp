/*
 * duel.cpp
 *
 *  Created on: 2010-5-2
 *      Author: Argon
 */

#include "duel.h"
#include "interpreter.h"
#include "field.h"
#include "card.h"
#include "effect.h"
#include "group.h"
#include "ocgapi.h"
#include <memory.h>

duel::duel(script_reader sreader_, card_reader creader_, message_handler mhandler_) {
	sreader = sreader_;
	creader = creader_;
	mhandler = mhandler_;
	lua = new interpreter(this);
	game_field = new field(this);
	game_field->temp_card = new_card(0);
	clear_buffer();
}
duel::~duel() {
	for(auto cit = cards.begin(); cit != cards.end(); ++cit)
		delete *cit;
	for(auto git = groups.begin(); git != groups.end(); ++git)
		delete *git;
	for(auto eit = effects.begin(); eit != effects.end(); ++eit)
		delete *eit;
	delete lua;
	delete game_field;
}
void duel::clear() {
	for(auto cit = cards.begin(); cit != cards.end(); ++cit)
		delete *cit;
	for(auto git = groups.begin(); git != groups.end(); ++git)
		delete *git;
	for(auto eit = effects.begin(); eit != effects.end(); ++eit)
		delete *eit;
	delete game_field;
	cards.clear();
	groups.clear();
	effects.clear();
	game_field = new field(this);
	game_field->temp_card = new_card(0);
}
card* duel::new_card(uint32 code) {
	card* pcard = new card(this);
	cards.insert(pcard);
	if (code)
	{
		if (creader)
			read_duel_card(this, code, &(pcard->data));
		else
			read_card(code, &(pcard->data));
	}
	pcard->data.code = code;
	lua->register_card(pcard);
	return pcard;
}
group* duel::register_group(group* pgroup) {
	groups.insert(pgroup);
	if(lua->call_depth)
		sgroups.insert(pgroup);
	lua->register_group(pgroup);
	return pgroup;
}
group* duel::new_group() {
	group* pgroup = new group(this);
	return register_group(pgroup);
}
group* duel::new_group(card* pcard) {
	group* pgroup = new group(this, pcard);
	return register_group(pgroup);
}
group* duel::new_group(const card_set& cset) {
	group* pgroup = new group(this, cset);
	return register_group(pgroup);
}
effect* duel::new_effect() {
	effect* peffect = new effect(this);
	effects.insert(peffect);
	lua->register_effect(peffect);
	return peffect;
}
void duel::delete_card(card* pcard) {
	cards.erase(pcard);
	delete pcard;
}
void duel::delete_group(group* pgroup) {
	lua->unregister_group(pgroup);
	groups.erase(pgroup);
	sgroups.erase(pgroup);
	delete pgroup;
}
void duel::delete_effect(effect* peffect) {
	lua->unregister_effect(peffect);
	effects.erase(peffect);
	delete peffect;
}
int32 duel::read_buffer(byte* buf) {
	memcpy(buf, buffer, bufferlen);
	return bufferlen;
}
void duel::release_script_group() {
	for(auto sit = sgroups.begin(); sit != sgroups.end(); ++sit) {
		group* pgroup = *sit;
		if(pgroup->is_readonly == 0) {
			lua->unregister_group(pgroup);
			groups.erase(pgroup);
			delete pgroup;
		}
	}
	sgroups.clear();
}
void duel::restore_assumes() {
	for(auto sit = assumes.begin(); sit != assumes.end(); ++sit)
		(*sit)->assume_type = 0;
	assumes.clear();
}
void duel::write_buffer32(uint32 value) {
	*((uint32*)bufferp) = value;
	bufferp += 4;
	bufferlen += 4;
}
void duel::write_buffer16(uint16 value) {
	*((uint16*)bufferp) = value;
	bufferp += 2;
	bufferlen += 2;
}
void duel::write_buffer8(uint8 value) {
	*((uint8*)bufferp) = value;
	bufferp += 1;
	bufferlen += 1;
}
void duel::clear_buffer() {
	bufferlen = 0;
	bufferp = buffer;
}
void duel::set_responsei(uint32 resp) {
	game_field->returns.ivalue[0] = resp;
}
void duel::set_responseb(byte* resp) {
	memcpy(game_field->returns.bvalue, resp, 64);
}
int32 duel::get_next_integer(int32 l, int32 h) {
	return (int32) (random.real() * (h - l + 1)) + l;
}
