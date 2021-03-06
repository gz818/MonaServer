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

#pragma once

#include "Mona/Mona.h"
#include "Mona/Peer.h"
#include "Mona/Invoker.h"
#include "Mona/Expirable.h"

namespace Mona {

class Sessions;
class Protocol;
class Session : virtual Object, public Expirable<Session> {
	friend class Sessions;

private:
	const std::shared_ptr<Peer> _pPeer; // before "peer" member to be created before it!!

public:
	virtual ~Session();

	UInt32				id()	const { return _id; }
	const std::string&	name()	const;
	
	template<typename ProtocolType = Protocol>
	ProtocolType& protocol() { return (ProtocolType&)_protocol; }

	Peer&				peer;
	Invoker&			invoker;

	bool				dumpJustInDebug;

	const bool			died;

	void dumpResponse(const UInt8* data, UInt32 size, bool justInDebug=false) { Writer::DumpResponse(data, size, peer.address, justInDebug); }

	template<typename DecodingType>
	void decode(const std::shared_ptr<DecodingType>& pDecoding) {
		shareThis(pDecoding->_expirableSession);
		Exception ex;
		_pDecodingThread = invoker.poolThreads.enqueue<DecodingType>(ex, pDecoding, _pDecodingThread);
		if (ex)
			ERROR("Impossible to decode packet of protocol ", protocolName(), " on session ", name(), ", ", ex.error());
	}

	template<typename DecodingType>
	void decode(const std::shared_ptr<DecodingType>& pDecoding,const SocketAddress& address) {
		pDecoding->_address.set(address);
		decode(pDecoding);
	}

	virtual void		receive(PacketReader& packet) { receiveWithoutFlush(packet); flush(); }
	virtual void		receive(PacketReader& packet, const SocketAddress& address);

	virtual void		manage() {}
	virtual void		kill();
	virtual void		flush() { peer.writer().flush(); }

protected:
	Session(Protocol& protocol,Invoker& invoker, const char* name=NULL);
	Session(Protocol& protocol, Invoker& invoker, const std::shared_ptr<Peer>& pPeer, const char* name = NULL);

	void				receiveWithoutFlush(PacketReader& packet);
	virtual void		packetHandler(PacketReader& packet)=0;

private:
	const std::string&  protocolName();

	PoolThread*					_pDecodingThread;
	mutable std::string			_name;
	UInt32						_id;
	Sessions*					_pSessions; // !NULL if managed by Sessions!
	UInt8						_sessionsOptions;
	Protocol&					_protocol;
};


} // namespace Mona
