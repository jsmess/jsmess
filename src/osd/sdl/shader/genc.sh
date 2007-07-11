#! /bin/sh

##
## VSH
##

echo "const char glsl_general_vsh_src[] =" > glsl_general.vsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_general.vsh >> glsl_general.vsh.c
echo ";" >> glsl_general.vsh.c

##
## IDX16 FSH 
##

echo "const char glsl_idx16_lut_fsh_src[] =" > glsl_idx16_lut.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_idx16_lut.fsh >> glsl_idx16_lut.fsh.c
echo ";" >> glsl_idx16_lut.fsh.c

echo "const char glsl_idx16_lut_bilinear_fsh_src[] =" > glsl_idx16_lut_bilinear.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_idx16_lut_bilinear.fsh >>  glsl_idx16_lut_bilinear.fsh.c
echo ";" >> glsl_idx16_lut_bilinear.fsh.c

echo "const char glsl_idx16_lut_conv3x3_fsh_src[] =" > glsl_idx16_lut_conv3x3.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_idx16_lut_conv3x3.fsh >>  glsl_idx16_lut_conv3x3.fsh.c
echo ";" >> glsl_idx16_lut_conv3x3.fsh.c

##
## RGB 32 FSH LUT
##

echo "const char glsl_rgb32_lut_fsh_src[] =" > glsl_rgb32_lut.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_lut.fsh >>  glsl_rgb32_lut.fsh.c
echo ";" >> glsl_rgb32_lut.fsh.c

echo "const char glsl_rgb32_lut_bilinear_fsh_src[] =" > glsl_rgb32_lut_bilinear.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_lut_bilinear.fsh >>  glsl_rgb32_lut_bilinear.fsh.c
echo ";" >> glsl_rgb32_lut_bilinear.fsh.c

echo "const char glsl_rgb32_lut_conv3x3_fsh_src[] =" > glsl_rgb32_lut_conv3x3.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_lut_conv3x3.fsh >>  glsl_rgb32_lut_conv3x3.fsh.c
echo ";" >> glsl_rgb32_lut_conv3x3.fsh.c


##
## RGB 32 FSH DIRECT
##

echo "const char glsl_rgb32_dir_fsh_src[] =" > glsl_rgb32_dir.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_dir.fsh >>  glsl_rgb32_dir.fsh.c
echo ";" >> glsl_rgb32_dir.fsh.c

echo "const char glsl_rgb32_dir_bilinear_fsh_src[] =" > glsl_rgb32_dir_bilinear.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_dir_bilinear.fsh >>  glsl_rgb32_dir_bilinear.fsh.c
echo ";" >> glsl_rgb32_dir_bilinear.fsh.c

echo "const char glsl_rgb32_dir_conv3x3_fsh_src[] =" > glsl_rgb32_dir_conv3x3.fsh.c
sed -e 's/^/"/g' -e 's/$/\\n"/g' glsl_rgb32_dir_conv3x3.fsh >>  glsl_rgb32_dir_conv3x3.fsh.c
echo ";" >> glsl_rgb32_dir_conv3x3.fsh.c

