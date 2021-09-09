#include "Serialization.h"
#include "ryml.hpp"
//TODO name to importer
//TODO overriding existing importer checks

SerializationContext SerializedObjectImporterBase::CreateContext(AssetDatabase_TextImporterHandle& handle) {
	return SerializationContext(handle.yaml);
}