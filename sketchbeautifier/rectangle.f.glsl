uniform sampler2D qt_Texture0;
varying vec4 qt_TexCoord0;

void main(void)
{
   precision mediump float;

    uniform float time;
    uniform vec2 mouse;
    uniform vec2 resolution;

                       // x1  y1   x2   y2
        vec4 rect = vec4(0.2, 0.3, 0.4, 0.5);
        vec2 hv = step(rect.xy, position) * step(position, rect.zw);
        float onOff = hv.x * hv.y;

        gl_FragColor = mix(vec4(0,0,0,0), vec4(1,0,0,0), onOff);
    }
}
