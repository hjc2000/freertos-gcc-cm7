add_library(${ProjectName} OBJECT)
target_import_src(${ProjectName})

target_import_bsp_interface(${ProjectName} PUBLIC)
