attribute vec4 qt_Vertex;
attribute vec4 qt_MultiTexCoord0;
uniform mat4 qt_ModelViewProjectionMatrix;
varying vec4 qt_TexCoord0;

void main(void)
{
    uniform vec2 resolution;
    uniform float time;
    uniform vec2 mouse;

    gl_Position = ( gl_FragCoord.xy / resolution.xy ) + mouse / 4.0;

}
