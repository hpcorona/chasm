
set(constant $ENV{CHC_RES_NAME})
set(filename $ENV{CHC_RES_PATH})
set(output $ENV{CHC_RES_OUTPUT})

if (EXISTS ${filename})
    file(WRITE ${output} "")

    file(READ ${filename} filedata HEX)

    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})

    file(APPEND ${output} "const unsigned char ${constant}[] = {${filedata}};\nconst unsigned ${constant}_size = sizeof(${constant});\nchar* get_${constant}() { return (char*)${constant}; } unsigned int get_${constant}_size() { return ${constant}_size; }")
endif()
