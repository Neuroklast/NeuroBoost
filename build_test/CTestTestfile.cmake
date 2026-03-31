# CMake generated Testfile for 
# Source directory: /home/runner/work/NeuroBoost/NeuroBoost
# Build directory: /home/runner/work/NeuroBoost/NeuroBoost/build_test
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(DSP_Test_Unity_Gain "/home/runner/work/NeuroBoost/NeuroBoost/build_test/bin/NeuroBoost_DSP_Test" "100")
set_tests_properties(DSP_Test_Unity_Gain PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;57;add_test;/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;0;")
add_test(DSP_Test_Half_Gain "/home/runner/work/NeuroBoost/NeuroBoost/build_test/bin/NeuroBoost_DSP_Test" "50")
set_tests_properties(DSP_Test_Half_Gain PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;59;add_test;/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;0;")
add_test(DSP_Test_Double_Gain "/home/runner/work/NeuroBoost/NeuroBoost/build_test/bin/NeuroBoost_DSP_Test" "200")
set_tests_properties(DSP_Test_Double_Gain PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;61;add_test;/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;0;")
add_test(DSP_Test_Mute "/home/runner/work/NeuroBoost/NeuroBoost/build_test/bin/NeuroBoost_DSP_Test" "0")
set_tests_properties(DSP_Test_Mute PROPERTIES  _BACKTRACE_TRIPLES "/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;63;add_test;/home/runner/work/NeuroBoost/NeuroBoost/CMakeLists.txt;0;")
