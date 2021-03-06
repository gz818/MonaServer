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

#include "Mona/Peer.h"
#include "Mona/Group.h"
#include "Mona/Handler.h"
#include "Mona/Util.h"
#include "Mona/Logs.h"

using namespace std;


namespace Mona {

class Member : virtual Object {
public:
	Member(Writer* pWriter) : pWriter(pWriter){}
	Writer*			pWriter;
};


Peer::Peer(Handler& handler) : _handler(handler), connected(false), relayable(false) {
}

Peer::~Peer() {
	unsubscribeGroups();
	if(relayable)
		_handler.relay.remove(*this);
	for(auto& it: _ices) {
		((Peer*)it.first)->_ices.erase(this);
		delete it.second;
	}
}


bool Peer::exchangeMemberId(Group& group,Peer& peer,Writer* pWriter) {
	if (pWriter) {
		pWriter->writeMember(peer);
		return true;
	}
	auto it = peer._groups.find(&group);
	if(it==peer._groups.end()) {
		CRITIC("A peer in a group without have its _groups collection associated")
		return false;
	}
	return it->second->writeMember(*this);
}

Group& Peer::joinGroup(const UInt8* id,Writer* pWriter) {
	// create invoker.groups if need
	Group& group(((Entities<Group>&)_handler.groups).create(id));

	if (group.count() > 0) {
		// If  group includes already members, give 6 last members to the new comer
		UInt8 count=6;
		auto it = group.end();
		do {
			Client& client(*(--it)->second);
			if(client==this->id || !exchangeMemberId(group,(Peer&)client,pWriter))
				continue;
			if(--count==0)
				break;
		} while(it!=group.begin());
	}


	// group._clients and this->_groups insertions
	auto it = _groups.lower_bound(&group);
	if(it!=_groups.end() && it->first==&group)
		return group;

	group.add(*this);
	_groups.emplace_hint(it,&group,pWriter);
	onJoinGroup(group);
	return group;
}


void Peer::unjoinGroup(Group& group) {
	auto it = _groups.lower_bound(&group);
	if (it == _groups.end() || it->first != &group)
		return;
	onUnjoinGroup(*it->first);
	_groups.erase(it);
}

void Peer::unsubscribeGroups() {
	for (auto& it : _groups)
		onUnjoinGroup(*it.first);
	_groups.clear();
}

ICE& Peer::ice(const Peer& peer) {
	map<const Peer*,ICE*>::iterator it = _ices.begin();
	while(it!=_ices.end()) {
		if(it->first == &peer) {
			it->second->setCurrent(*this);
			return *it->second;
		}
		if(it->second->obsolete()) {
			delete it->second;
			_ices.erase(it++);
			continue;
		}
		if(it->first>&peer)
			break;
		++it;
	}
	if(it!=_ices.begin())
		--it;
	ICE& ice = *_ices.emplace_hint(it,&peer,new ICE(*this,peer,_handler.relay))->second; // is offer
	((Peer&)peer)._ices[this] = &ice;
	return ice;
}

/// EVENTS ////////


void Peer::onHandshake(UInt32 attempts,set<SocketAddress>& addresses) {
	_handler.onHandshake(protocol, address, path, properties(), attempts, addresses);
}

void Peer::onRendezVousUnknown(const UInt8* peerId,set<SocketAddress>& addresses) {
	_handler.onRendezVousUnknown(protocol, peerId, addresses);
}

void Peer::onConnection(Exception& ex, Writer& writer,DataReader& parameters,DataWriter& response) {
	if(!connected) {
		_pWriter = &writer;

		writer.state(Writer::CONNECTING);
		_handler.onConnection(ex, *this,parameters,response);
		if (!ex)
			(bool&)connected = ((Clients&)_handler.clients).add(ex,*this);
		if (ex) {
			writer.state(Writer::CONNECTED,true);
			_pWriter = NULL;
			return;
		}
		writer.state(Writer::CONNECTED);
	} else
		ERROR("Client ", Util::FormatHex(id, ID_SIZE, _handler.buffer), " seems already connected!")
}

void Peer::onDisconnection() {
	if (!connected)
		return;
	_pWriter = NULL;
	(bool&)connected = false;
	if (!((Clients&)_handler.clients).remove(*this))
		ERROR("Client ", Util::FormatHex(id, ID_SIZE, _handler.buffer), " seems already disconnected!");
	_handler.onDisconnection(*this);
}

void Peer::onMessage(Exception& ex, const string& name,DataReader& reader,UInt8 responseType) {
	if(connected)
		_handler.onMessage(ex, *this, name, reader, responseType);
	else
		ERROR("RPC client before connection")
}

void Peer::onJoinGroup(Group& group) {
	if(!connected)
		return;
	_handler.onJoinGroup(*this,group);
}

void Peer::onUnjoinGroup(Group& group) {
	// group._clients suppression (this->_groups suppression must be done by the caller of onUnjoinGroup)
	auto itPeer = group.find(id);
	if (itPeer == group.end()) {
		ERROR("Peer::onUnjoinGroup on a group which don't know this peer");
		return;
	}
	itPeer = group.remove(itPeer);

	if(connected)
		_handler.onUnjoinGroup(*this,group);

	if (group.count() == 0) {
		((Entities<Group>&)_handler.groups).erase(group.id);
		return;
	}
	if (itPeer == group.end())
		return;

	// if a peer disconnects of one group, give to its following peer the 6th preceding peer
	Peer& followingPeer((Peer&)*itPeer->second);
	UInt8 count=6;
	while(--count!=0 && itPeer!=group.begin())
		--itPeer;
	if(count==0)
		((Peer*)itPeer->second)->exchangeMemberId(group,followingPeer,NULL);
}

bool Peer::onPublish(const Publication& publication,string& error) {
	if(connected)
		return _handler.onPublish(*this,publication,error);
	WARN("Publication client before connection")
	error = "Client must be connected before publication";
	return false;
}

void Peer::onUnpublish(const Publication& publication) {
	if(connected) {
		_handler.onUnpublish(*this,publication);
		return;
	}
	WARN("Unpublication client before connection")
}

bool Peer::onSubscribe(const Listener& listener,string& error) {
	if(connected)
		return _handler.onSubscribe(*this,listener,error);
	WARN("Subscription client before connection")
	error = "Client must be connected before subscription";
	return false;
}

void Peer::onUnsubscribe(const Listener& listener) {
	if(connected) {
		_handler.onUnsubscribe(*this,listener);
		return;
	}
	WARN("Unsubscription client before connection")
}

bool Peer::onRead(Exception& ex, FilePath& filePath,DataReader& parameters,DataWriter& properties) {
	if(connected)
		return _handler.onRead(ex, *this, filePath,parameters,properties);
	ERROR("Resource '",filePath.path(),"' access by a not connected client")
	return false;
}

void Peer::onDataPacket(const Publication& publication,DataReader& packet) {
	if(connected) {
		_handler.onDataPacket(*this,publication,packet);
		return;
	}
	WARN("DataPacket client before connection")
}

void Peer::onAudioPacket(const Publication& publication,UInt32 time,PacketReader& packet) {
	if(connected) {
		_handler.onAudioPacket(*this,publication,time,packet);
		return;
	}
	WARN("AudioPacket client before connection")
}

void Peer::onVideoPacket(const Publication& publication,UInt32 time,PacketReader& packet) {
	if(connected) {
		_handler.onVideoPacket(*this,publication,time,packet);
		return;
	}
	WARN("VideoPacket client before connection")
}

void Peer::onFlushPackets(const Publication& publication) {
	if(connected) {
		_handler.onFlushPackets(*this,publication);
		return;
	}
	WARN("FlushPackets client before connection")
}





} // namespace Mona
