add_executable(vorbis_test util.c util.h write_read.c write_read.h test.c)
target_link_libraries(vorbis_test PRIVATE Vorbis::vorbisenc $<$<BOOL:${HAVE_LIBM}>:m>)
add_test(NAME vorbis_test COMMAND vorbis_test)
