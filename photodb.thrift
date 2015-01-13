include "type.thrift"

namespace java photodb.thrift
namespace cpp photodb
namespace php photodb

service MonitorService {
	string getStat(1:required string param),
	string executeCmd(1:required string param),
}

enum PUT_OP {
	ADD,
	ADD_OR_UPDATE
}

service PhotoDB {

	i32 ping(),
	type.MetaValueResult getMeta(1:required type.Key key),
	// width: scaling witdh, = 0 for no scaling
	type.ImgValueResult getImg(1:required type.Key key, 2:required i32 width),
	
	i64 putMeta(1:required type.Key key, 2:required type.MetaValue value, 3:required PUT_OP put_op),
	i64 putImg(1:required type.Key key, 2:required type.ImgValue value),

	i32 exist(1:required type.Key key),
	i64 remove(1:required type.Key key),

	oneway void ow_putMeta(1:required type.Key key, 2:required type.MetaValue value, 3:required PUT_OP put_op),
	oneway void ow_putImg(1:required type.Key key, 2:required type.ImgValue value),
	oneway void ow_remove(1:required type.Key key),
}