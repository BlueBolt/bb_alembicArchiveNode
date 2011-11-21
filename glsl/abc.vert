varying vec3 normal, lightDir;


void main()
{	
	lightDir = normalize(vec3(gl_LightSource[0].position));
	normal = normalize(gl_NormalMatrix * gl_Normal);

	gl_Position = ftransform();
}