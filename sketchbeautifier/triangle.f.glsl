uniform sampler2D qt_Texture0;
varying vec4 qt_TexCoord0;
varying vec3 f_color;
#define Thickness 0.05


void main(void) {
    gl_FragColor = vec4(f_color.x, f_color.y, f_color.z, 1.0);

}
