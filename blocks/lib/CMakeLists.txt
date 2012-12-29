include(GrPlatform) #define LIB_SUFFIX


add_library(op25 SHARED abstract_data_unit.cc  CMakeLists.txt  data_unit.cc data_unit_handler.cc dummy_imbe_decoder.cc hdu.cc
imbe_decoder.cc  imbe_decoder_factory.cc  ldu1.cc ldu2.cc logfile_du_handler.cc offline_imbe_decoder.cc  op25_decoder_bf.cc  op25_decoder_ff.cc op25_fsk4_demod_ff.cc op25_fsk4_slicer_fb.cc op25_pcap_source_b.cc p25cai_du_handler.cc pdu.cc pickle.cc
snapshot_du_handler.cc software_imbe_decoder.cc tdu.cc tsbk.cc value_string.cc vc55_imbe_decoder.cc voice_data_unit.cc voice_du_handler.cc )

target_link_libraries(op25 ${Boost_LIBRARIES} ${GRUEL_LIBRARIES} ${GNURADIO_CORE_LIBRARIES})

set_target_properties(op25 PROPERTIES DEFINE_SYMBOL "op25_EXPORTS")

install(TARGETS op25 
    LIBRARY DESTINATION lib${LIB_SUFFIX} # .so/.dylib file
    ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib file
    RUNTIME DESTINATION bin              # .dll file
)

find_package(Boost COMPONENTS unit_test_framework)

include(GrTest)
set(GR_TEST_TARGET_DEPS gnuradio-pocsag)

add_definitions(-DBOOST_TEST_DYN_LINK -DBOOST_TEST_MAIN)

