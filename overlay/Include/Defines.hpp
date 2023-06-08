#pragma once

#define AUTO auto
#define EXTERN extern
#define EXPORT __declspec(dllexport)
#define SMART_PTR_ALLOC(HOLDER) HOLDER = new std::remove_pointer<decltype(HOLDER)>::type();
#define SMART_PTR_CREATE(NAME, HOLDER) const auto NAME = std::make_unique< std::remove_pointer<decltype(HOLDER)>::type >(); \
	HOLDER = NAME.get()

#define SHPR_GET_FILE_PATH(X) G::StorageHpr.get(_XS(X))
#define SHPR_GET_DIR_PATH(X) G::StorageHpr.get_dir(_XS(X))
