#version 330

layout (triangles_adjacency) in;
layout (triangle_strip, max_vertices = 27) out;

uniform vec3 EyePos;
uniform mat4 mvMatrix;
uniform mat4 mvpMatrix;
uniform mat4 norMatrix;
uniform vec4 lightPos;
uniform float creaseWidth;
uniform float silhoueWidth;

in VertexData
{
    int id;
    vec3 normal;
} vertex[];

out float diffTerm;
out vec2 tcoord;
out float renderEdg;


void lighting(vec4 posn, vec3 normal)
{
	vec4 posnEye = mvMatrix * posn;
	vec4 normalEye = norMatrix * vec4(normal, 0);
	vec3 unitNormal = normalize(normalEye.xyz);
	vec3 lgtVec = normalize(lightPos.xyz - posnEye.xyz); 

	diffTerm = dot(lgtVec, unitNormal);
}

vec3 faceNormal(vec4 a, vec4 b, vec4 c)
{
	vec3 ab = b.xyz - a.xyz;
	vec3 ac = c.xyz - a.xyz;
	vec3 face_normal = normalize(cross(ab, ac));

	return face_normal;
}

vec2[3] texturing(vec4 primitive[6])
{
	vec2 outputs[3];
	float AB, AC, AD;

	vec3 normA = faceNormal(primitive[0], primitive[2], primitive[4]);
	vec3 normB = faceNormal(primitive[0], primitive[1], primitive[2]);
	vec3 normC = faceNormal(primitive[2], primitive[3], primitive[4]);
	vec3 normD = faceNormal(primitive[4], primitive[5], primitive[0]);

	// Find edge with largest dihedral angle
	//AB = acos(dot(normA, normB));
	//AC = acos(dot(normA, normC));
	//AD = acos(dot(normA, normD));

	AB = acos((normA.x*normB.x+normA.y*normB.y+normA.z*normB.z) / (sqrt(pow(normA.x,2)+pow(normA.y,2)+pow(normA.z,2)) * sqrt(pow(normB.x,2)+pow(normB.y,2)+pow(normB.z,2))));
	AC = acos((normA.x*normC.x+normA.y*normC.y+normA.z*normC.z) / (sqrt(pow(normA.x,2)+pow(normA.y,2)+pow(normA.z,2)) * sqrt(pow(normC.x,2)+pow(normC.y,2)+pow(normC.z,2))));
	AD = acos((normA.x*normD.x+normA.y*normD.y+normA.z*normD.z) / (sqrt(pow(normA.x,2)+pow(normA.y,2)+pow(normA.z,2)) * sqrt(pow(normD.x,2)+pow(normD.y,2)+pow(normD.z,2))));

	if (AB > AC && AB > AD) // AB
	{
		outputs[0] = vec2(0.5, 0);
		outputs[1] = vec2(0, 0);
		outputs[2] = vec2(0.25, 0.5);
	} else if (AC > AB && AC > AD) // AC
	{
		outputs[1] = vec2(0.5, 0);
		outputs[2] = vec2(0, 0);
		outputs[0] = vec2(0.25, 0.5);
	} else if (AD > AB && AD > AC) // AD
	{
		outputs[2] = vec2(0.5, 0);
		outputs[0] = vec2(0, 0);
		outputs[1] = vec2(0.25, 0.5);
	} else { // base case
		outputs[0] = vec2(0, 0);
		outputs[1] = vec2(0.5, 0);
		outputs[2] = vec2(0.25, 0.5);
	}

	return outputs;
}

void creaseEdgStrip(vec4 verts[4], vec3 v)
{
	float d1 = 0.05f, d2 = (creaseWidth * 0.5f), d3 = 0.25f; //d1 = 0.0001f
	vec3 posn[6];

	vec3 u = normalize(verts[3].xyz - verts[0].xyz);

	vec3 lft = normalize(normalize(verts[1].xyz - verts[0].xyz) + normalize(verts[1].xyz - verts[3].xyz));
	vec3 rgt = normalize(normalize(verts[2].xyz - verts[0].xyz) + normalize(verts[2].xyz - verts[3].xyz));

	vec3 Pmid = verts[0].xyz + (d1 * v) - (u * d3); // P
	vec3 Qmid = verts[3].xyz + (d1 * v) + (u * d3); // Q

	vec3 lftNorm = faceNormal(verts[0], verts[1], verts[3]);
	vec3 rgtNorm = faceNormal(verts[0], verts[3], verts[2]);

	posn[0] = Pmid + (d2 * lft) + (d1 * lftNorm); // P1
	posn[1] = Qmid + (d2 * lft) + (d1 * lftNorm); // Q1

	posn[2] = Pmid; // P2
	posn[3] = Qmid; // Q2

	posn[4] = Pmid + (d2 * rgt) + (d1 * rgtNorm); // P3
	posn[5] = Qmid + (d2 * rgt) + (d1 * rgtNorm); // Q3

	renderEdg = 1.0f;
	
	for (int i = 0; i < 6; i++)
	{
		gl_Position = mvpMatrix * vec4(posn[i], 1);
		EmitVertex();
	}
	EndPrimitive();
}

void genCreaseEdgs(vec4 primitive[6], float threshold)
{
	// Creates a triangle strip for crease edges of the current primitive.
	
	vec3 v;

	vec3 normA = faceNormal(primitive[0], primitive[2], primitive[4]);
	vec3 normB = faceNormal(primitive[0], primitive[1], primitive[2]);
	vec3 normC = faceNormal(primitive[2], primitive[3], primitive[4]);
	vec3 normD = faceNormal(primitive[4], primitive[5], primitive[0]);

	float angle = threshold * (3.14159f / 180.0f);

	int AB = acos(dot(normA, normB))>angle?1:0;
	int AC = acos(dot(normA, normC))>angle?1:0;
	int AD = acos(dot(normA, normD))>angle?1:0;

	if (AB == 1) // 0 - 2
	{
		v = normalize(normA + normB);

		vec4 verts[4] = {primitive[0], primitive[1], primitive[4], primitive[2]};

		creaseEdgStrip(verts, v);
	}

	if (AC == 1) // 2 - 4
	{
		v = normalize(normA + normC);

		vec4 verts[4] = {primitive[2], primitive[3], primitive[0], primitive[4]};

		creaseEdgStrip(verts, v);
	}

	if (AD == 1) // 4 - 0
	{
		v = normalize(normA + normD);

		vec4 verts[4] = {primitive[4], primitive[5], primitive[2], primitive[0]};

		creaseEdgStrip(verts, v);
	}
}

vec4 projectVec(vec4 a, vec4 b)
{
	vec4 outVec;

	outVec = a - (((dot(a,b)) / (pow(length(b),2))) * b);

	return normalize(outVec);
}

void silhouetteEdgStrip(vec4 v, vec4 verts[2])
{
	float d1 = 0.000001f, d2 = silhoueWidth, d3 = 1.0f; //d3 = 0.0025f;
	vec4 u, outVerts[4];


	vec3 up = normalize(vec3(mvMatrix[0][1], mvMatrix[1][1], mvMatrix[2][1]));
	vec3 right = normalize(vec3(mvMatrix[0][0], mvMatrix[1][0], mvMatrix[2][0]));
	vec4 eyeNorm = normalize(vec4(cross(up, right),0));

	u = normalize(verts[1] - verts[0]);
	u = projectVec(u, eyeNorm);
	v = projectVec(v, eyeNorm);


	outVerts[0] = verts[0] + (d1 * v) - (d3 * u); // P1
	outVerts[1] = verts[1] + (d1 * v) + (d3 * u); // Q1
	outVerts[2] = verts[0] + (d2 * v) - (d3 * u); // P2
	outVerts[3] = verts[1] + (d2 * v) + (d3 * u); // Q2
	
	for (int i = 0; i < 4; i++)
	{
		gl_Position = mvpMatrix * outVerts[i];

		renderEdg = 1.0f;

		EmitVertex();
	}
	EndPrimitive();
}

void genSilhouetteEdgs(vec4 primitive[6])
{
	// Creates a triangle strip for silhouette edges of the current primitive.
	vec4 normA = norMatrix * normalize(vec4(faceNormal(primitive[0], primitive[2], primitive[4]), 0));
	vec4 normB = norMatrix * normalize(vec4(faceNormal(primitive[0], primitive[1], primitive[2]), 0));
	vec4 normC = norMatrix * normalize(vec4(faceNormal(primitive[2], primitive[3], primitive[4]), 0));
	vec4 normD = norMatrix * normalize(vec4(faceNormal(primitive[4], primitive[5], primitive[0]), 0));

	int AB = (normA.z>0 && normB.z<0) || (normA.z<0 && normB.z>0)?1:0;
	int AC = (normA.z>0 && normC.z<0) || (normA.z<0 && normC.z>0)?1:0;
	int AD = (normA.z>0 && normD.z<0) || (normA.z<0 && normD.z>0)?1:0;

	if (AB == 1) 
	{
		vec4 primVerts[2] = {primitive[0], primitive[2]};
		
		vec4 v = normalize(vec4(faceNormal(primitive[0], primitive[2], primitive[4]) + faceNormal(primitive[0], primitive[1], primitive[2]),0));

		silhouetteEdgStrip(v, primVerts);
	}

	if (AC == 1) 
	{
		vec4 primVerts[2] = {primitive[2], primitive[4]};

		vec4 v = normalize(vec4(faceNormal(primitive[0], primitive[2], primitive[4]) + faceNormal(primitive[2], primitive[3], primitive[4]),0));

		silhouetteEdgStrip(v, primVerts);
	}

	if (AD == 1) 
	{
		vec4 primVerts[2] = {primitive[4], primitive[0]};

		vec4 v = normalize(vec4(faceNormal(primitive[0], primitive[2], primitive[4]) + faceNormal(primitive[4], primitive[5], primitive[0]),0));

		silhouetteEdgStrip(v, primVerts);
	}
}

void main()
{
	// Extract positions from input
	vec4 primitive[6];
	for (int i = 0; i < 6; i++)
	{
		primitive[i] = gl_in[i].gl_Position;
	}

	// Emit each vertex of the main triangle
	vec4 posn[3] = {primitive[0], primitive[2], primitive[4]};
	vec3 norm[3] = {vertex[0].normal, vertex[2].normal, vertex[4].normal};

	vec2 texcds[3] = texturing(primitive);

	//vec2 texcds[3] = {vec2(0, 0), vec2(1, 0), vec2(0.5, 1)};
	
	
	for (int i = 0; i < 3; i++)
	{
		gl_Position = mvpMatrix * posn[i];

		lighting(posn[i], norm[i]);
		tcoord = texcds[i];

		renderEdg = 0.0f;

		EmitVertex();
	}
	EndPrimitive();
	

	// Emit triangle strip for crease edges
	genCreaseEdgs(primitive, 31.5f);

	// Emit triangle strip for silhouett edges
	genSilhouetteEdgs(primitive);
}