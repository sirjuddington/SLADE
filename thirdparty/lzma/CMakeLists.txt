cmake_minimum_required( VERSION 2.4 )

if( CMAKE_COMPILER_IS_GNUC )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fomit-frame-pointer" )
endif( CMAKE_COMPILER_IS_GNUC )

set( LZMA_FILES
	C/7zAlloc.c
	C/7zBuf.c
	C/7zBuf2.c
	C/7zCrc.c
	C/7zCrcOpt.c
	C/7zDec.c
	C/7zFile.c
	C/7zIn.c
	C/7zStream.c
    C/Alloc.c
    C/Bcj2.c
	C/Bra.c
    C/Bra86.c
	C/BraIA64.c
	C/CpuArch.c
	C/Delta.c
    C/LzFind.c
    C/LzFindMt.c
    C/LzmaDec.c
    C/LzmaEnc.c
	C/Lzma2Dec.c
	C/Lzma2Enc.c
	C/Lzma86Dec.c
	C/Lzma86Enc.c
    C/LzmaLib.c
	C/MtCoder.c
	C/Ppmd7.c
	C/Ppmd7Dec.c
	C/Ppmd7Enc.c
	C/Sha256.c )

if( WIN32 )
	set( LZMA_FILES ${LZMA_FILES} C/Threads.c )
endif( WIN32 )

add_library( lzma ${LZMA_FILES} )
target_link_libraries( lzma )
