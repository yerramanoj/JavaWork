#include "Box.h"
#include "../Util/GLUtil.h"
#include "../Math/TransformVertexBuf.h"

Box::Box() : Shape(Shape::BOX)
{
	_w = 0;
	_h = 0;
	_d = 0;
}

Box::Box(float x, float y, float z, float w, float h, float d) : Shape(Shape::BOX)
{
	m[12] = x;
	m[13] = y;
	m[14] = z;

	_w = w;
	_h = h;
	_d = d;
}

Box::Box(float* mat, CVector3 size) : Shape(Shape::BOX)
{
	memcpy(m, mat, 16*4);

	_w = size.x;
	_h = size.y;
	_d = size.z;
}

Box::Box(Box* box) : Shape(Shape::BOX)
{
	memcpy(m, box->m, 16*4);

	_w = box->GetSize().x;
	_h = box->GetSize().y;
	_d = box->GetSize().z;

	_id = box->GetID();
}

void Box::Set(Box* box)
{
	memcpy(m, box->m, 16*4);

	_w = box->GetSize().x;
	_h = box->GetSize().y;
	_d = box->GetSize().z;
}

void Box::SetPos(float x, float y, float z)
{
	m[12] = x;
	m[13] = y;
	m[14] = z;
}

CVector3 Box::GetPos()
{
	return CVector3(m[12], m[13], m[14]);
}

CVector3 Box::GetSize()
{
	return CVector3(_w, _h, _d);
}

void Box::SetSize(float w, float h, float d)
{
	if(w > 0)	_w = w;
	if(h > 0)	_h = h;
	if(d > 0)	_d = d;
}

float Box::Volume()
{
	return _w*_h*_d;
}

Box Box::CalcAABB(float* vertexBuf, int arrSize)
{
	float minX = vertexBuf[0];
	float maxX = vertexBuf[0];
	float minY = vertexBuf[1];
	float maxY = vertexBuf[1];
	float minZ = vertexBuf[2];
	float maxZ = vertexBuf[2];

	for(int i=3; i<arrSize; i+=3)
	{
		float x = vertexBuf[i+0];
		float y = vertexBuf[i+1];
		float z = vertexBuf[i+2];

		if(x < minX) minX = x;
		if(x > maxX) maxX = x;

		if(y < minY) minY = y;
		if(y > maxY) maxY = y;
			
		if(z < minZ) minZ = z;
		if(z > maxZ) maxZ = z;
	}

	float w = abs( maxX - minX );
	float h = abs( maxY - minY );
	float d = abs( maxZ - minZ );

	float x = (minX+maxX)/2.0f;
	float y = (minY+maxY)/2.0f;
	float z = (minZ+maxZ)/2.0f;

	return Box(x, y, z, w, h, d);
}

vector<CVector3> Box::GetAABBVertices()
{
	CVector3 pos = GetPos();
	CVector3 size = GetSize() * 0.5f;

	CVector3 minPos( pos.x - size.x, pos.y - size.y, pos.z - size.z);
	CVector3 maxPos( pos.x + size.x, pos.y + size.y, pos.z + size.z);

	vector<CVector3> pos3D;
	
	pos3D.push_back( CVector3(minPos.x, minPos.y, minPos.z));
	pos3D.push_back( CVector3(minPos.x, maxPos.y, minPos.z));
	pos3D.push_back( CVector3(maxPos.x, minPos.y, minPos.z));
	pos3D.push_back( CVector3(maxPos.x, maxPos.y, minPos.z));
	pos3D.push_back( CVector3(minPos.x, minPos.y, maxPos.z));
	pos3D.push_back( CVector3(minPos.x, maxPos.y, maxPos.z));
	pos3D.push_back( CVector3(maxPos.x, minPos.y, maxPos.z));
	pos3D.push_back( CVector3(maxPos.x, maxPos.y, maxPos.z));

	return pos3D;
}

Box Box::CalcBoundingBox(float* vertexBuf, int arrSize)
{
	float* localVertexBuf = new float[arrSize];
	memcpy(localVertexBuf, vertexBuf, arrSize*sizeof(float));

	CVector3 center = TransformVertexBuf::CalcCenter(localVertexBuf, arrSize);
	TransformVertexBuf::Subtract(localVertexBuf, arrSize, center);


	float delta = 30.0f;
	CVector3 start(0,0,0);
	CVector3 end(360,360,360);

	CVector3 rot(0,0,0);
	Box* prevBox = new Box(0,0,0, 0,0,0);
	bool once = true;

	for(int loop=0; loop<5; loop++)
	{
		if(loop != 0)
		{
			start = CVector3(rot.x-delta, rot.y-delta, rot.z-delta);
			end	  = CVector3(rot.x+delta, rot.y+delta, rot.z+delta);
		}

        if(loop == 1)		delta = 10;
        else if(loop == 2)	delta = 5;
        else if(loop == 3)	delta = 2;
        else if(loop == 4)	delta = 1;

		for(float zAng=start.z; zAng<end.z; zAng+=delta)
		{
			for(float yAng=start.y; yAng<end.y; yAng+=delta)
			{
				for(float xAng=start.x; xAng<end.x; xAng+=delta)
				{
					Box bBox = GetBoundingBoxAfterRotXYZ(localVertexBuf, arrSize, xAng, yAng, zAng);
			
					if(once || bBox.Volume() < prevBox->Volume())
					{
						prevBox->Set( &bBox );
						rot = CVector3(xAng, yAng, zAng);
						once = false;
					}
				}
			}
		}
	}

	delete[] localVertexBuf;

	GLMat mat;
	mat.glTranslatef(center.x, center.y, center.z);
	mat.glRotatef(-rot.x, 1,0,0);
	mat.glRotatef(-rot.y, 0,1,0);
	mat.glRotatef(-rot.z, 0,0,1);

	CVector3 trans = prevBox->GetPos();
	CVector3 size = prevBox->GetSize();
	mat.glTranslatef(trans.x, trans.y, trans.z);
	
	delete prevBox;

	Box returnBox(mat.m, size);

	return returnBox;
}


Box Box::GetBoundingBoxAfterRotXYZ(float* vertexBuf, int arrSize, float xAng, float yAng, float zAng)
{
	float cosOfXAng = cos(xAng * DEG_RAD);
	float cosOfYAng = cos(yAng * DEG_RAD);
	float cosOfZAng = cos(zAng * DEG_RAD);

	float sinOfXAng = sin(xAng * DEG_RAD);
	float sinOfYAng = sin(yAng * DEG_RAD);
	float sinOfZAng = sin(zAng * DEG_RAD);

	float minX = vertexBuf[0];
	float minY = vertexBuf[1];
	float minZ = vertexBuf[2];

	float maxX = vertexBuf[0];
	float maxY = vertexBuf[1];
	float maxZ = vertexBuf[2];

	for(int i=3; i<arrSize; i+=3)
	{
		float x = vertexBuf[i+0];
		float y = vertexBuf[i+1];
		float z = vertexBuf[i+2];

		float x1 = x;
		float y1 = y*cosOfXAng - z*sinOfXAng;
		float z1 = y*sinOfXAng + z*cosOfXAng;

		float y2 = y1;
		float z2 = z1*cosOfYAng - x1*sinOfYAng;
		float x2 = z1*sinOfYAng + x1*cosOfYAng;

		x = x2*cosOfZAng - y2*sinOfZAng;
		y = x2*sinOfZAng + y2*cosOfZAng;
		z = z2;

		if(x < minX)	minX = x;
		if(y < minY)	minY = y;
		if(z < minZ)	minZ = z;

		if(x > maxX)	maxX = x;
		if(y > maxY)	maxY = y;
		if(z > maxZ)	maxZ = z;
	}

	float x = (minX+maxX)/2;
	float y = (minY+maxY)/2;
	float z = (minZ+maxZ)/2;

	float w = abs(maxX-minX);
	float h = abs(maxY-minY);
	float d = abs(maxZ-minZ);

	return Box(x,y,z, w,h,d);
}


void Box::Draw()
{
	GLboolean isLightOn = GLUtil::GLEnable( GL_LIGHTING, false );

	float w = _w/2.0f;
	float h = _h/2.0f;
	float d = _d/2.0f;

	glPushMatrix();
	glMultMatrixf(m);

	glBegin(GL_QUADS);

	_randomColor.Reset();

	//Front face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, -h, +d);
	glVertex3f(+w, -h, +d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, +h, +d);
	glVertex3f(-w, +h, +d);

	//Back face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, -h, -d);
	glVertex3f(-w, +h, -d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, +h, -d);
	glVertex3f(+w, -h, -d);

	//Top face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, +h, -d);
	glVertex3f(-w, +h, +d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, +h, +d);
	glVertex3f(+w, +h, -d);
	
	//Bottom face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, -h, -d);
	glVertex3f(+w, -h, -d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, -h, +d);
	glVertex3f(-w, -h, +d);

	//Right face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, -h, -d);
	glVertex3f(+w, +h, -d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(+w, +h, +d);
	glVertex3f(+w, -h, +d);

	//Left face
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, -h, -d);
	glVertex3f(-w, -h, +d);
	glColorA(_randomColor.NextColor(), _randomColorAlpha);
	glVertex3f(-w, +h, +d);
	glVertex3f(-w, +h, -d);
	glEnd();

	glPopMatrix();

	GLUtil::GLEnable( GL_LIGHTING, isLightOn );
}

Box::~Box()
{
}
