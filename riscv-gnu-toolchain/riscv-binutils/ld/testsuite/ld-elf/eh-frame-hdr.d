#source: eh-frame-hdr.s
#ld: -e _start --eh-frame-hdr
#objdump: -hw
#alltargets: cfi shared
#...
  [0-9] .eh_frame_hdr +0*[12][048c] .*
#pass
