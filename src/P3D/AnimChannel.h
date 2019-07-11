#pragma once

#include <string>
#include <MemoryStream.h>

namespace Donut::P3D {

class AnimChannel {
public:
	AnimChannel() {}
	virtual void Read(MemoryStream& stream);
protected:
	uint32_t _version;
	std::string _parameter;
};

} // namespace Donut::P3D