/*
Copyright 2014 Mona
mathieu.poux[a]gmail.com
jammetthomas[a]gmail.com

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License received along this program for more
details (or else see http://www.gnu.org/licenses/).

This file is a part of Mona.
*/

#include "Mona/Invoker.h"
#include "Mona/Logs.h"

using namespace std;


namespace Mona {


Invoker::Invoker(UInt32 socketBufferSize,UInt16 threads) : poolThreads(threads),relay(poolBuffers,poolThreads,socketBufferSize),sockets(*this,poolBuffers,poolThreads,socketBufferSize),publications(_publications),_nextId(0) {
	DEBUG(poolThreads.threadsAvailable()," threads available in the server poolthreads");
		
}


Invoker::~Invoker() {
	// delete groups
	for(auto& it : groups)
		delete it.second;
}

shared_ptr<FlashStream>& Invoker::createFlashStream(Peer& peer) {
	map<UInt32, shared_ptr<FlashStream> >::iterator it;
	do {
		it = _streams.lower_bound((++_nextId) == 0 ? ++_nextId : _nextId);
	} while (it != _streams.end() && it->first == _nextId);
	shared_ptr<FlashStream> pStream(new FlashStream(_nextId,*this, peer));
	return _streams.emplace_hint(it, _nextId, pStream)->second;
}

FlashStream& Invoker::flashStream(UInt32 id,Peer& peer,shared_ptr<FlashStream>& pStream) {
	// flash main stream
	if (id == 0) {
		if (!pStream)
			pStream.reset(new FlashMainStream(*this,peer));
		return *pStream;
	}
	if (pStream) {
		// search inside pStream
		FlashStream* pResult = pStream->stream(id);
		if (pResult)
			return *pResult;	
	}
	// search in streams list
	auto it = _streams.lower_bound(id);
	if (it != _streams.end() && id == it->first) {
		if (!pStream)
			pStream = it->second;
		return *it->second;
	}
	// return pStream passed or a create a new FlashStream
	if (pStream)
		return *pStream;
	pStream.reset(new FlashStream(id, *this, peer));
	_streams.emplace_hint(it, id, pStream);
	return *pStream;
}

Publication* Invoker::publish(Exception& ex, Peer& peer,const string& name) {
	auto it(_publications.emplace(piecewise_construct,forward_as_tuple(name),forward_as_tuple(name)).first);
	Publication* pPublication = &it->second;
	
	pPublication->start(ex, peer);
	if (ex) {
		if (!pPublication->publisher() && pPublication->listeners.count() == 0)
			_publications.erase(it);
		return NULL;
	}
	return pPublication;
}

void Invoker::unpublish(Peer& peer,const string& name) {
	auto it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The publication '",name,"' doesn't exist, unpublish useless");
		return;
	}
	Publication& publication(it->second);
	publication.stop(peer);
	if(!publication.publisher() && publication.listeners.count()==0)
		_publications.erase(it);
}

Listener* Invoker::subscribe(Exception& ex, Peer& peer,const string& name,Writer& writer,double start) {
	auto it(_publications.emplace(piecewise_construct,forward_as_tuple(name),forward_as_tuple(name)).first);
	Publication& publication(it->second);
	Listener* pListener = publication.addListener(ex, peer,writer,start==-3000 ? true : false);
	if (ex) {
		if(!publication.publisher() && publication.listeners.count()==0)
			_publications.erase(it);
	}
	return pListener;
}

void Invoker::unsubscribe(Peer& peer,const string& name) {
	auto it = _publications.find(name);
	if(it == _publications.end()) {
		DEBUG("The publication '",name,"' doesn't exists, unsubscribe useless");
		return;
	}
	Publication& publication(it->second);
	publication.removeListener(peer);
	if(!publication.publisher() && publication.listeners.count()==0)
		_publications.erase(it);
}


} // namespace Mona
