$input a_position, a_color0, a_normal 
$output v_position, v_normal

#include <bgfx_shader.sh>

void main()
{
    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
    v_position = a_position;
    v_normal = a_normal;
}
