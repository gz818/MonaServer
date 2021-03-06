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
#include "Mona/PoolBuffers.h"


namespace Mona {

class PoolBuffer : virtual Object {
public:
	PoolBuffer(const PoolBuffers& poolBuffers,UInt32 size=0) : _size(size),poolBuffers(poolBuffers),_pBuffer(NULL) {}
	virtual ~PoolBuffer() { release(); }

	bool	empty() const { return !_pBuffer || _pBuffer->size()==0; }
	Buffer* operator->() const { if (!_pBuffer) _pBuffer=poolBuffers.beginBuffer(_size);  return _pBuffer; }
	Buffer& operator*() const { if (!_pBuffer) _pBuffer=poolBuffers.beginBuffer(_size);  return *_pBuffer; }

	
	void	swap(PoolBuffer& buffer) { std::swap(buffer._pBuffer,_pBuffer); }
	void	release() { if (!_pBuffer) return; poolBuffers.endBuffer(_pBuffer); _pBuffer = NULL; }

	const PoolBuffers&	poolBuffers;

private:
	mutable Buffer*		_pBuffer;
	UInt32				_size;

};


} // namespace Mona
