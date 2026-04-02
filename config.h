#define PLUG_NAME "NeuroBoost"
#define PLUG_MFR "Neuroklast"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'NrBs'
#define PLUG_MFR_ID 'Nklt'
#define PLUG_URL_STR "https://github.com/Neuroklast/NeuroBoost"
#define PLUG_EMAIL_STR "contact@neuroklast.com"
#define PLUG_COPYRIGHT_STR "Copyright 2025 Neuroklast"
#define PLUG_CLASS_NAME NeuroBoost

#define BUNDLE_NAME "NeuroBoost"
#define BUNDLE_MFR "Neuroklast"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "NeuroBoost"

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 1
#define PLUG_DOES_MIDI_IN 1
#define PLUG_DOES_MIDI_OUT 1
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 1
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 900
#define PLUG_HEIGHT 600
#define PLUG_MIN_WIDTH 600
#define PLUG_MIN_HEIGHT 400
#define PLUG_MAX_WIDTH 1920
#define PLUG_MAX_HEIGHT 1080
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 1

#define AUV2_ENTRY NeuroBoost_Entry
#define AUV2_ENTRY_STR "NeuroBoost_Entry"
#define AUV2_FACTORY NeuroBoost_Factory
#define AUV2_VIEW_CLASS NeuroBoost_View
#define AUV2_VIEW_CLASS_STR "NeuroBoost_View"

#define AAX_TYPE_IDS 'NBF1', 'NBF2'
#define AAX_TYPE_IDS_AUDIOSUITE 'NBA1', 'NBA2'
#define AAX_PLUG_MFR_STR "Neuroklast"
#define AAX_PLUG_NAME_STR "NeuroBoost\\nNBST"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Instrument|Generator"

#define CLAP_MANUAL_URL "https://github.com/Neuroklast/NeuroBoost/wiki"
#define CLAP_SUPPORT_URL "https://github.com/Neuroklast/NeuroBoost/issues"
#define CLAP_DESCRIPTION "Deterministic and probabilistic MIDI sequencer with fractal math"
#define CLAP_FEATURES "note-effect"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

// Linux: iPlug2 does not define BUNDLE_ID or APP_GROUP_ID for the Linux target.
// Define them here so that MakeConfig() in IPlug_include_in_plug_src.h compiles.
#ifdef __linux__
  #ifndef BUNDLE_ID
    #define BUNDLE_ID BUNDLE_DOMAIN "." BUNDLE_MFR ".vst3." BUNDLE_NAME
  #endif
  #ifndef APP_GROUP_ID
    #define APP_GROUP_ID ""
  #endif
#endif
