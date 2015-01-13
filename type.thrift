namespace java photodb.thrift
namespace cpp photodb
namespace php photodb

typedef i64 Key
typedef list<Key> ListKey

typedef i16 FieldId
typedef list<FieldId> ListField

struct MetaValue
{
	1:optional i64 key,
	2:optional string contentType,
	3:optional string ext,
	4:optional i64 createdTime,
	5:optional i64 updatedTime,
	6:optional string etag,
	7:optional i32 width,
	8:optional i32 height,
}

struct ImgValue
{
	1:optional binary img,
}

struct MetaValueResult {
	1:required i32 error,
	2:optional MetaValue value,
}

struct ImgValueResult {
	1:required i32 error,
	2:optional ImgValue value,
}