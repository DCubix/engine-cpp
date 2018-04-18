#include "api.h"

NS_BEGIN

namespace api {

	i32 getDataTypeSize(DataType dt) {
		switch (dt) {
			case DataType::Byte:
			case DataType::UByte:
				return 1;
			case DataType::Float:
			case DataType::Int:
			case DataType::UInt:
				return 4;
			case DataType::Short:
			case DataType::UShort:
				return 2;
		}
	}

}

NS_END
