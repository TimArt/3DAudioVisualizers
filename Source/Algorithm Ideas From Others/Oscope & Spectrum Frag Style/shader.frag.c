#version 150 core
uniform vec3  iResolution; 
uniform float iGlobalTime;
uniform float Spectrum[256];
uniform float Wavedata[256];
in vec4 gl_FragCoord;
out vec4 gl_FragColor;

void InterpolateValue(in float index, out float value) {
	float norm = 255.0/iResolution.x*index;
	int Floor = int(floor(norm));
	int Ceil = int(ceil(norm));
	value = mix(Wavedata[Floor], Wavedata[Ceil], fract(norm));
}

#define THICKNESS 0.02
void main()
{
	float time = iGlobalTime;
	float x = gl_FragCoord.x / iResolution.x;
	float y = gl_FragCoord.y / iResolution.y;
	
	float wave = 0;
	InterpolateValue(x*iResolution.x, wave);
	
	wave = 0.5-wave/3; //centers wave
	
	float r = abs(THICKNESS/((wave-y)));
	gl_FragColor = vec4(r-abs(r*0.2*sin(time/5)), r-abs(r*0.2*sin(time/7)), r-abs(r*0.2*sin(time/9)), 0);
}