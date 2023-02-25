#version 330

uniform sampler2D pencilStroke[8];
uniform int shadingMode;
uniform int tType;

in float diffTerm;
in vec2 tcoord;
in float renderEdg;


float lerp(float t, float a, float b) 
{
	return clamp((t - a) / (b - a), 0, 1);
}

vec4 blendMaterial(float diffTerm)
{
	float bamt = 0.05f;														// blend amount
	float h[5] = { -1.0f, 0.2f, 0.5f, 0.7f, 0.9f };							// texture ranges
	float shade1, shade2, shade3, shade4, blend1, blend2, blend3, blend4;	// blending co-efficients

	// calculate blending co-efficients
	blend1 = lerp(diffTerm, h[4] - bamt, h[4] + bamt);	// White & lightest texture
	blend2 = lerp(diffTerm, h[3] - bamt, h[3] + bamt);
	blend3 = lerp(diffTerm, h[2] - bamt, h[2] + bamt);
	blend4 = lerp(diffTerm, h[1] - bamt, h[1] + bamt);	// Darkest texture
	
	if (diffTerm > h[3] - bamt && diffTerm <= h[4] + bamt) // Lightest texture
	{
		shade1 = (1.0f - blend1) - (1.0f - blend2);
	}
	if (diffTerm > h[2] - bamt && diffTerm <= h[3] + bamt) 
	{
		shade2 = (1.0f - blend2) - (1.0f - blend3);
	}
	if (diffTerm > h[1] - bamt && diffTerm <= h[2] + bamt) 
	{
		shade3 = (1.0f - blend3) - (1.0f - blend4);
	}
	if (diffTerm >= h[0] && diffTerm <= h[1] + bamt) // Darkest texture
	{
		shade4 = 1.0f - blend4;
	}
	
	// Texture blending
	vec4 texColor1 = texture(pencilStroke[0+(4*tType)], tcoord); // Lightest texture
	vec4 texColor2 = texture(pencilStroke[1+(4*tType)], tcoord);
	vec4 texColor3 = texture(pencilStroke[2+(4*tType)], tcoord);
	vec4 texColor4 = texture(pencilStroke[3+(4*tType)], tcoord); // Darkest texture
		
	vec4 material = ( blend1 * vec4(1.0)	// White
					+ shade1 * texColor1	// Lightest texture
					+ shade2 * texColor2 
					+ shade3 * texColor3 
					+ shade4 * texColor4);	// Darkest texture

	// Texture to display
	return material;
}

void main() 
{
	// Two-tone shading
	if (shadingMode == 0) 
	{
		gl_FragColor = (diffTerm <= 0.7f)?vec4(0.0f, 0.5f, 0.5f, 1.0f):vec4(0.0f, 1.0f, 1.0f, 1.0f);
	}

	// Pencil shading
	if (shadingMode == 1) 
	{
		gl_FragColor = blendMaterial(diffTerm);

		/*
		if(diffTerm > 0.9) // white
		{
			gl_FragColor = vec4(1.0f);
		}

		if(diffTerm > 0.7 && diffTerm <= 0.9) // lightest texture
		{
			gl_FragColor = texture(pencilStroke[0], tcoord);
		}

		if(diffTerm > 0.5 && diffTerm <= 0.7)
		{
			gl_FragColor = texture(pencilStroke[1], tcoord);
		}

		if(diffTerm > 0.2 && diffTerm <= 0.5)
		{
			gl_FragColor = texture(pencilStroke[2], tcoord);
		}

		if(diffTerm >= -1.0f && diffTerm <= 0.2)
		{
			gl_FragColor = texture(pencilStroke[3], tcoord);
		}
		*/
	}
	
	// Render black edges
	if (renderEdg == 1.0f)
	{
		gl_FragColor = vec4(0);
	} 
}
