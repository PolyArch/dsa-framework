This is the optimized EMTV 3D Reconstruction Code developed by the team of UCLA.

Authors List:
"William Hsu" <willhsu@mii.ucla.edu>
"Shiwen Shen" <sshen@mii.ucla.edu>
"Ming Yan" <basca.yan@gmail.com>

Dept of Radiological Sciences, UCLA
Dept. of Eletrical Engineering, UCLA
Dept. of Computer Science, UCLA
Dept. of Mathematics, UCLA

Prerequisites:
This code has been developed and tested on Windows and Linux platforms with GCC version 4.8.

Instructions:
1. In the root directory, run 'make clean'
2. Run 'make'
3. Go to the bin directory
4. Run the following command
./emtv3d -t 16 -z 8 -l 0.75 -a 25 -g 200 -R 1.4 -f ../data/PAT1_CT_SINOGRAM_POS -i ../data/PAT1_CT_SINOGRAM_RAW -o ../result/PAT1_CT_RECON_EMTV -c -0.25 -N 0 -s 11731 -p 23465

Please see the provided Rich Text Format document for detailed instructions and an explanation of the command line triggers.