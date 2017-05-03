//  Copyright © 2016 Fatih Gazimzyanov. Contacts: virgil7g@gmail.com
//  Copyright © 2013 Tyler Tesch.       Contacts: https://www.youtube.com/user/Advenio4821

//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//          http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and

#version 150 core
uniform vec3  iResolution;
uniform float iGlobalTime;
uniform float spectrum[256];
uniform float waveData[256];
uniform float lowFilter;
uniform float highFilter;
in vec4 gl_FragCoord;
out vec4 gl_FragColor;

void InterpolateValue(in float index, out float value) {
	float norm = 255.0 / iResolution.x * index;
	int Floor = int(floor(norm));
	int Ceil = int(ceil(norm));
	value = mix(waveData[Floor], waveData[Ceil], fract(norm));
}

#define THICKNESS 0.02
void main()
{
	float time = iGlobalTime;
	float x = gl_FragCoord.x / iResolution.x;
	float y = gl_FragCoord.y / iResolution.y;

	float wave = 0;
	InterpolateValue(x * iResolution.x, wave);

	wave = 0.5 - wave / 3; //centers wave

	float r = abs(THICKNESS/(wave - y));
	if ((gl_FragCoord.x > lowFilter / 128.0 * iResolution.x * 0.5 ) &&
	    (gl_FragCoord.x < highFilter / 128.0 * iResolution.x * 0.5)) {
	    gl_FragColor = vec4(
	        r - abs(r * 0.2 * sin(time / 5)), r - abs(r * 0.2 * sin(time / 7)), r - abs(r * 0.2 * sin(time / 9)), 0);
	} else {
	    r = r / 40;
	  	gl_FragColor = vec4(
	  	    r - abs(r * 0.2 * sin(time / 5)), r - abs(r * 0.2 * sin(time / 7)), r - abs(r * 0.2 * sin(time / 9)), 0);
	}
}