
void TexturedMeshVisualization::UpdateMainFrameSize() {
	if (displayMode == ONE_REGION_DISPLAY) {
		_screenWidth = screenWidth;
		_screenHeight = screenHeight;
	}
	else if (displayMode == TWO_REGION_DISPLAY) {
		_screenWidth = screenWidth / 2;
		_screenHeight = screenHeight;
	}
	else if (displayMode == THREE_REGION_DISPLAY) {
		_screenWidth = screenWidth * 2 / 3;
		_screenHeight = screenHeight;
	}
	else if (displayMode == FOUR_REGION_DISPLAY) {
		_screenWidth = screenWidth * 2 / 5;
		_screenHeight = screenHeight;
	}
}

float TexturedMeshVisualization::imageToScreenScale(void) const {
	return std::min< float >(float(screenWidth) / float(textureImage.width()), float(screenHeight) / float(textureImage.height())) * xForm.zoom;
}
Point< float, 2 > TexturedMeshVisualization::imageToScreen(float px, float py) const
{
	float ip[] = { px, py }, ic[] = { float(textureImage.width()) / 2 + xForm.offset[0], float(textureImage.height()) / 2 - xForm.offset[1] }, sc[] = { float(screenWidth) / 2, float(screenHeight) / 2 };
	float scale = imageToScreenScale();
	Point< float, 2 > sp;
	sp[0] = sc[0] + (ip[0] - ic[0])*scale;
	sp[1] = sc[1] - (ip[1] - ic[1])*scale;
	return sp;
}

void TexturedMeshVisualization::SetupOffScreenBuffer() {
	// The depth buffer texture
	glGenTextures(1, &offscreen_depth_texture);
	glBindTexture(GL_TEXTURE_2D, offscreen_depth_texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, offscreen_frame_width, offscreen_frame_height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// The color buffer texture
	glGenTextures(1, &offscreen_color_texture);
	glBindTexture(GL_TEXTURE_2D, offscreen_color_texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, offscreen_frame_width, offscreen_frame_height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


	// Create and set up the FBO
	glGenFramebuffers(1, &offscreen_framebuffer_handle);
	glBindFramebuffer(GL_FRAMEBUFFER, offscreen_framebuffer_handle);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, offscreen_depth_texture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, offscreen_color_texture, 0);
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result == GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer is complete.\n");
	}
	else {
		printf("Framebuffer is not complete.\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TexturedMeshVisualization::RenderOffScreenBuffer(Image<Point3D<float>> & image) {
	if (!offscreen_framebuffer_handle) SetupOffScreenBuffer();
	glViewport(0, 0, offscreen_frame_width, offscreen_frame_height);
	glBindFramebuffer(GL_FRAMEBUFFER, offscreen_framebuffer_handle);
	int windowScreenWidth = screenWidth;
	int windowScreenHeight = screenHeight;
	screenWidth = offscreen_frame_width;
	screenHeight = offscreen_frame_height;
	display();
	screenWidth = windowScreenWidth;
	screenHeight = windowScreenHeight;
	glFlush();

	//Save color buffer to image
	Pointer(float) GLColorBuffer = AllocPointer< float >(sizeof(float) * 3 * offscreen_frame_width * offscreen_frame_height);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glReadPixels(0, 0, offscreen_frame_width, offscreen_frame_height, GL_RGB, GL_FLOAT, GLColorBuffer);
	glFinish();
	image.resize(offscreen_frame_width, offscreen_frame_height);
	for (int i = 0; i<offscreen_frame_width; i++) for (int j = 0; j<offscreen_frame_height; j++)  for (int c = 0; c<3; c++) {
		image(i, j)[c] = GLColorBuffer[c + i * 3 + (offscreen_frame_height - 1 - j) * offscreen_frame_width * 3];
	}
	FreePointer(GLColorBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, screenWidth, screenHeight);
}


//void TexturedMeshVisualization::WriteSceneConfigurationCallBack(Visualization* v, const char* prompt) {
//	const TexturedMeshVisualization* av = (TexturedMeshVisualization*)v;
//	FILE * file;
//	file = fopen(prompt, "wb");
//	fwrite(&av->screenWidth, sizeof(int), 1, file);
//	fwrite(&av->screenHeight, sizeof(int), 1, file);
//	fwrite(&av->camera.position, sizeof(Point3D<double>), 1, file);
//	fwrite(&av->camera.forward, sizeof(Point3D<double>), 1, file);
//	fwrite(&av->camera.right, sizeof(Point3D<double>), 1, file);
//	fwrite(&av->camera.up, sizeof(Point3D<double>), 1, file);
//	fwrite(&av->zoom, sizeof(float), 1, file);
//	fclose(file);
//}
//
//void TexturedMeshVisualization::ReadSceneConfigurationCallBack(Visualization* v, const char* prompt) {
//	TexturedMeshVisualization* av = (TexturedMeshVisualization*)v;
//	FILE * file;
//	file = fopen(prompt, "rb");
//	if (!file) {
//		printf("Camera Configuration File Not Valid \n");
//	}
//	else {
//		fread(&av->screenWidth, sizeof(int), 1, file);
//		fread(&av->screenHeight, sizeof(int), 1, file);
//		fread(&av->camera.position, sizeof(Point3D<double>), 1, file);
//		fread(&av->camera.forward, sizeof(Point3D<double>), 1, file);
//		fread(&av->camera.right, sizeof(Point3D<double>), 1, file);
//		fread(&av->camera.up, sizeof(Point3D<double>), 1, file);
//		fread(&av->zoom, sizeof(float), 1, file);
//		//av->offscreen_frame_height = av->screenHeight;
//		//av->offscreen_frame_width = av->screenWidth;
//		fclose(file);
//	}
//}

void TexturedMeshVisualization::WriteSceneConfigurationCallBack(Visualization* v, const char* prompt) {
	const TexturedMeshVisualization* av = (TexturedMeshVisualization*)v;
	FILE * file;
	file = fopen(prompt, "wb");
	fwrite(&av->screenWidth, sizeof(int), 1, file);
	fwrite(&av->screenHeight, sizeof(int), 1, file);
	fwrite(&av->camera.position, sizeof(glm::vec3), 1, file);
	fwrite(&av->camera.direction, sizeof(glm::vec3), 1, file);
	fwrite(&av->camera.right, sizeof(glm::vec3), 1, file);
	fwrite(&av->camera.up, sizeof(glm::vec3), 1, file);
	fwrite(&av->zoom, sizeof(float), 1, file);
	fclose(file);
}

void TexturedMeshVisualization::ReadSceneConfigurationCallBack(Visualization* v, const char* prompt) {
	TexturedMeshVisualization* av = (TexturedMeshVisualization*)v;
	FILE * file;
	file = fopen(prompt, "rb");
	if (!file) {
		printf("Camera Configuration File Not Valid \n");
	}
	else {
		fread(&av->screenWidth, sizeof(int), 1, file);
		fread(&av->screenHeight, sizeof(int), 1, file);
		fread(&av->camera.position, sizeof(glm::vec3), 1, file);
		fread(&av->camera.direction, sizeof(glm::vec3), 1, file);
		fread(&av->camera.right, sizeof(glm::vec3), 1, file);
		fread(&av->camera.up, sizeof(glm::vec3), 1, file);
		fread(&av->zoom, sizeof(float), 1, file);
		fclose(file);
	}
}

void TexturedMeshVisualization::ScreenshotCallBack(Visualization* v, const char* prompt) {
	Image<Point3D<float>> image;
	TexturedMeshVisualization* av = (TexturedMeshVisualization*)v;
	av->RenderOffScreenBuffer(image);
	image.write(prompt);
}

void TexturedMeshVisualization::UpdateVertexBuffer() {
	if (!glIsBuffer(vertexBuffer)) {
		glGenBuffers(1, &vertexBuffer);
	}
	if (!glIsBuffer(normalBuffer)) {
		glGenBuffers(1, &normalBuffer);
	}
	if (!glIsBuffer(colorBuffer)) {
		glGenBuffers(1, &colorBuffer);
	}
	if (!glIsBuffer(coordinateBuffer)) {
		glGenBuffers(1, &coordinateBuffer);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * triangles.size() * sizeof(Point3D<float>), &vertices[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * triangles.size() * sizeof(Point3D<float>), &normals[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * triangles.size() * sizeof(Point3D<float>), &colors[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, coordinateBuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * triangles.size() * sizeof(Point2D<float>), &textureCoordinates[0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &vertexHandle);
	glBindVertexArray(vertexHandle);

	{//Texture
	 // Vertex position
		glEnableVertexAttribArray(0);
		glBindVertexBuffer(0, vertexBuffer, 0, sizeof(GLfloat) * 3);
		glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribBinding(0, 0);

		// Vertex texture
		glEnableVertexAttribArray(1);
		glBindVertexBuffer(1, coordinateBuffer, 0, sizeof(GLfloat) * 2);
		glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribBinding(1, 1);

		// Vertex Normal
		glEnableVertexAttribArray(2);
		glBindVertexBuffer(2, normalBuffer, 0, sizeof(GLfloat) * 3);
		glVertexAttribFormat(2, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexAttribBinding(2, 2);
	}

	glBindVertexArray(0);
}

void TexturedMeshVisualization::UpdateFaceBuffer() {
	if (!glIsBuffer(faceBuffer)) {
		glGenBuffers(1, &faceBuffer);
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);
	TriangleIndex *_triangles = new TriangleIndex[triangles.size()];

	for (int i = 0; i<triangles.size(); i++) for (int j = 0; j<3; j++) _triangles[i][j] = 3 * i + j;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangles.size() * sizeof(int) * 3, _triangles, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	delete[] _triangles;
}

void TexturedMeshVisualization::UpdateTextureBuffer() {
	if (!glIsBuffer(textureBuffer)) {
		glGenTextures(1, &textureBuffer);
	}

	int height = textureImage.height();
	int width = textureImage.width();
	unsigned char * colors = new unsigned char[height*width * 3];
	for (int j = 0; j < height; j++) for (int i = 0; i < width; i++) for (int c = 0; c < 3; c++) colors[3 * (width*j + i) + c] = (unsigned char)(std::min<double>((double)(textureImage(i, j)[c])*255.0, 255.0));
	glBindTexture(GL_TEXTURE_2D, textureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)&colors[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	delete colors;
}


void TexturedMeshVisualization::SetLightingData() {
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

	//lightPosition[0] = -camera.forward[0];
	//lightPosition[1] = -camera.forward[1];
	//lightPosition[2] = -camera.forward[2];
	//lightPosition[3] = 0.f;

	lightPosition[0] = -camera.direction[0];
	lightPosition[1] = -camera.direction[1];
	lightPosition[2] = -camera.direction[2];
	lightPosition[3] = 0.f;

	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, shapeAmbient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, shapeDiffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, shapeSpecular);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shapeSpecularShininess);
}

//void TexturedMeshVisualization::SetGeometryCamera() {
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	float ar = 1.f;
//	gluPerspective(30, ar, nearPlane, farPlane);
//
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//	gluLookAt(
//		camera.position[0], camera.position[1], camera.position[2],
//		camera.position[0] + camera.forward[0], camera.position[1] + camera.forward[1], camera.position[2] + camera.forward[2],
//		camera.up[0], camera.up[1], camera.up[2]
//	);
//}

void TexturedMeshVisualization::SetGeometryCamera() {
	camera.updateTransformations();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(&camera.projection_matrix[0][0]);
	//gluPerspective(camera.heightAngle, 1.0, camera.nearest_plane, camera.farthest_plane);
	//Draw Camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(&camera.world_to_camera[0][0]);
	//gluLookAt(camera.position[0], camera.position[1], camera.position[2], camera.position[0] + camera.direction[0], camera.position[1] + camera.direction[1], camera.position[2] + camera.direction[2], camera.up[0], camera.up[1], camera.up[2]);
}

void TexturedMeshVisualization::SetTextureCamera() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.5, 0.5, -0.5, 0.5, -1, 1);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void TexturedMeshVisualization::PhongShading(GLuint & textureBufferId) {
	GLSLProgram * current_program = normalProgram;
	glUseProgram(current_program->getHandle());

	current_program->setUniform("eye_projection", camera.projection_matrix);
	current_program->setUniform("world_to_eye", camera.world_to_camera);

	current_program->setUniform("light_direction", camera.direction);
	current_program->setUniform("light_diffuse", light_diffuse);
	current_program->setUniform("light_specular", light_specular);
	current_program->setUniform("light_ambient", light_ambient);
	current_program->setUniform("specular_falloff", specular_fallof);

	current_program->setUniform("normal_texture", 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureBufferId);

	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);

	//		glPolygonOffset( 30.f , 30.f );

	glBindVertexArray(vertexHandle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits);
	glDrawElements(GL_TRIANGLES, 3 * triangles.size(), GL_UNSIGNED_INT, 0);
	glDisable(GL_POLYGON_OFFSET_FILL);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void TexturedMeshVisualization::DrawGeometry(GLuint & textureBufferId, bool phongShading, bool modulateLight) {
	SetGeometryCamera();

	glEnable(GL_TEXTURE_2D);

	if (phongShading) {
		PhongShading(textureBufferId);
	}
	else {
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, coordinateBuffer);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);

		glBindTexture(GL_TEXTURE_2D, textureBufferId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);
		if (modulateLight) SetLightingData();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulateLight ? GL_MODULATE : GL_DECAL);

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits);
		glDrawElements(GL_TRIANGLES, (GLsizei)(triangles.size() * 3), GL_UNSIGNED_INT, NULL);
		glDisable(GL_POLYGON_OFFSET_FILL);
		if (modulateLight) glDisable(GL_LIGHTING);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	}

	glDisable(GL_TEXTURE_2D);


	if (showBoundaryEdges) {
		glDisable(GL_LIGHTING);
		glLineWidth(lineWidth);
		glColor3f(1.0, 1.0, 1.0);
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glBegin(GL_LINES);
		for (int i = 0; i < boundaryEdgeVertices.size(); i++) glVertex3f(boundaryEdgeVertices[i][0], boundaryEdgeVertices[i][1], boundaryEdgeVertices[i][2]);
		glEnd();
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}

	if (showEdges) {

		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ARRAY_BUFFER, coordinateBuffer);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);

		glDisable(GL_LIGHTING);
		glLineWidth(lineWidth);
		glColor3f(0.125, 0.125, 0.125);
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, (GLsizei)(triangles.size() * 3), GL_UNSIGNED_INT, NULL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	}
}

//void TexturedMeshVisualization::DrawGeometry(GLuint & textureBufferId, bool modulateLight) {
//	SetGeometryCamera();
//
//	glEnable(GL_TEXTURE_2D);
//
//	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
//	glEnableClientState(GL_VERTEX_ARRAY);
//	glVertexPointer(3, GL_FLOAT, 0, NULL);
//
//	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
//	glEnableClientState(GL_NORMAL_ARRAY);
//	glNormalPointer(GL_FLOAT, 0, NULL);
//
//	glBindBuffer(GL_ARRAY_BUFFER, coordinateBuffer);
//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glTexCoordPointer(2, GL_FLOAT, 0, NULL);
//
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);
//
//	glBindTexture(GL_TEXTURE_2D, textureBufferId);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, useNearestSampling ? GL_NEAREST : GL_LINEAR);
//	if (modulateLight) SetLightingData();
//	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, modulateLight ? GL_MODULATE : GL_DECAL);
//
//	glEnable(GL_POLYGON_OFFSET_FILL);
//	glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits);
//	glDrawElements(GL_TRIANGLES, (GLsizei)(triangles.size() * 3), GL_UNSIGNED_INT, NULL);
//	glDisable(GL_POLYGON_OFFSET_FILL);
//	if (modulateLight) glDisable(GL_LIGHTING);
//	glBindTexture(GL_TEXTURE_2D, 0);
//
//
//	if (showEdges) {
//		glDisable(GL_LIGHTING);
//		glLineWidth(lineWidth);
//		glColor3f(0.125, 0.125, 0.125);
//		glDisable(GL_MULTISAMPLE);
//		glEnable(GL_BLEND);
//		glEnable(GL_LINE_SMOOTH);
//		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//		glDepthMask(GL_FALSE);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//		glDrawElements(GL_TRIANGLES, (GLsizei)(triangles.size() * 3), GL_UNSIGNED_INT, NULL);
//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//
//		glDisable(GL_LINE_SMOOTH);
//		glDisable(GL_BLEND);
//		glDepthMask(GL_TRUE);
//	}
//
//	glDisableClientState(GL_VERTEX_ARRAY);
//	glDisableClientState(GL_NORMAL_ARRAY);
//	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
//
//
//	glDisable(GL_TEXTURE_2D);
//
//
//	if (showBoundaryEdges) {
//		glDisable(GL_LIGHTING);
//		glLineWidth(lineWidth);
//		glColor3f(0.8, 0.8, 0.8);
//		glDisable(GL_MULTISAMPLE);
//		glEnable(GL_BLEND);
//		glEnable(GL_LINE_SMOOTH);
//		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
//		glDepthMask(GL_FALSE);
//		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		glBegin(GL_LINES);
//		for (int i = 0; i < boundaryEdgeVertices.size(); i++) glVertex3f(boundaryEdgeVertices[i][0], boundaryEdgeVertices[i][1], boundaryEdgeVertices[i][2]);
//		glEnd();
//		glDisable(GL_LINE_SMOOTH);
//		glDisable(GL_BLEND);
//		glDepthMask(GL_TRUE);
//	}
//
//}
void TexturedMeshVisualization::DrawRegion(bool drawGeometry, GLuint & textureBufferId, bool phongShading, bool modulateLight) {
	if (drawGeometry)DrawGeometry(textureBufferId, phongShading, modulateLight);
	else DrawTexture(textureBufferId);
}

void TexturedMeshVisualization::DrawTexture(GLuint & textureBufferId) {
	SetTextureCamera();

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, textureBufferId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


	float screenScale = std::max<float>(screenWidth, screenHeight);

	Point2D<float> q;
	glBegin(GL_QUADS);
	{
		glTexCoord2d(0, 0);
		q = (Point2D<float>(-0.5, -0.5) + Point2D<float>(xForm.offset[0], xForm.offset[1]) / screenScale)*xForm.zoom;
		glVertex2f(q[0], q[1]);

		glTexCoord2d(1, 0);
		q = (Point2D<float>(0.5, -0.5) + Point2D<float>(xForm.offset[0], xForm.offset[1]) / screenScale)*xForm.zoom;
		glVertex2f(q[0], q[1]);

		glTexCoord2d(1, 1);
		q = (Point2D<float>(0.5, 0.5) + Point2D<float>(xForm.offset[0], xForm.offset[1]) / screenScale)*xForm.zoom;
		glVertex2f(q[0], q[1]);

		glTexCoord2d(0, 1);
		q = (Point2D<float>(-0.5, 0.5) + Point2D<float>(xForm.offset[0], xForm.offset[1]) / screenScale)*xForm.zoom;
		glVertex2f(q[0], q[1]);
	}
	glEnd();

	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (showEdges) {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();

		glScalef(xForm.zoom, xForm.zoom, 1.0);
		glTranslatef(xForm.offset[0] / screenScale, xForm.offset[1] / screenScale, 0.f);
		glTranslatef(-0.5, -0.5, 0.f);

		glBindBuffer(GL_ARRAY_BUFFER, coordinateBuffer);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_FLOAT, 0, NULL);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, faceBuffer);

		glLineWidth(lineWidth);
		glColor3f(0.125, 0.125, 0.125);
		glDisable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_LINE_SMOOTH);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDrawElements(GL_TRIANGLES, (GLsizei)(triangles.size() * 3), GL_UNSIGNED_INT, NULL);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);

		glDisableClientState(GL_VERTEX_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glPopMatrix();
	}
}



void TexturedMeshVisualization::display() {
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (displayMode == ONE_REGION_DISPLAY) {
		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, screenWidth, screenHeight);
		DrawRegion(showMesh, textureBuffer, false, true);
	}
	else if (displayMode == TWO_REGION_DISPLAY) {
		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, screenWidth / 2, screenHeight);
		DrawRegion(true, textureBuffer, false, true);

		glViewport(screenWidth / 2, 0, screenWidth / 2, screenHeight);
		DrawRegion(false, textureBuffer, false, true);

		glViewport(0, 0, screenWidth, screenHeight);
	}
}


void TexturedMeshVisualization::keyboardFunc(unsigned char key, int x, int y)
{

}

void TexturedMeshVisualization::mouseFunc(int button, int state, int x, int y)
{
	if (!showMesh) {
		newX = x; newY = y;

		scaling = panning = false;
		if (button == GLUT_LEFT_BUTTON) panning = true;
		else if (button == GLUT_RIGHT_BUTTON) scaling = true;
	}
	else {
		newX = x; newY = y;

		rotating = scaling = panning = false;
		if (button == GLUT_LEFT_BUTTON)
			if (glutGetModifiers() & GLUT_ACTIVE_CTRL) panning = true;
			else                                        rotating = true;
		else if (button == GLUT_RIGHT_BUTTON) scaling = true;
	}
}

void TexturedMeshVisualization::motionFunc(int x, int y)
{
	if (!showMesh){
		oldX = newX, oldY = newY, newX = x, newY = y;

		int imageSize = std::min< int >(screenWidth, screenHeight);
		if (panning) xForm.offset[0] -= (newX - oldX) / imageToScreenScale(), xForm.offset[1] += (newY - oldY) / imageToScreenScale();
		else
		{
			float dz = float(pow(1.1, double(newY - oldY) / 8));
			xForm.zoom *= dz;
		}
		glutPostRedisplay();
	}
	else {
		oldX = newX, oldY = newY, newX = x, newY = y;
		int screenSize = std::min< int >(screenWidth, screenHeight);
		float rel_x = (newX - oldX) / (float)screenSize * 2;
		float rel_y = (newY - oldY) / (float)screenSize * 2;
		float pRight = -rel_x * zoom, pUp = rel_y * zoom;
		float pForward = rel_y * zoom;
		float rRight = rel_y, rUp = rel_x;

		//if (rotating) camera.rotateUp(rUp), camera.rotateRight(rRight);
		//else if (scaling) camera.translate(camera.forward *pForward);
		//else if (panning) camera.translate(camera.right * pRight + camera.up * pUp);
		
		if (rotating) camera.rotateUp(center, rUp), camera.rotateRight(center, rRight);
		else if (scaling) camera.moveForward(pForward);
		else if (panning) camera.moveRight(pRight), camera.moveUp(pUp);

		glutPostRedisplay();
	}
}

TexturedMeshVisualization::TexturedMeshVisualization(){
	useNearestSampling = false;
	showEdges = false;
	showBoundaryEdges = false;
	showMesh = true;
	screenHeight = 800;
	screenWidth = 800;

	offscreen_frame_width = 1200;
	offscreen_frame_height = 1200;

	//nearPlane = 0.01f;
	//farPlane = 100.f;

	zoom = 1.f;
	rotating = scaling = panning = false;

	//camera.position = Point3D<float>(0.f, 0.f, -5.f);
	radius = 1.f;
	center = vec3(0.f, 0.f, 0.f);


	camera.heightAngle = 30;
	camera.aspectRatio = 1.0;
	//camera.aspectRatio = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
	camera.position = vec3(0.f, 0.f, 3.f);
	camera.direction = vec3(0.f, 0.f, -1.f);
	camera.up = vec3(0.f, 1.f, 0.f);
	camera.right = glm::cross(camera.direction, camera.up);
	camera.nearest_plane = radius*0.001f;
	camera.farthest_plane = radius*10.f;

	callBacks.push_back(KeyboardCallBack(this, 'C', "read camera", "File Name", ReadSceneConfigurationCallBack));
	callBacks.push_back(KeyboardCallBack(this, 'c', "save camera", "File Name", WriteSceneConfigurationCallBack));
	callBacks.push_back(KeyboardCallBack(this, 'e', "show edges", ShowEdgesCallBack));
	callBacks.push_back(KeyboardCallBack(this, 'm', "toggle mesh-atlas", ToggleVisualizationMode));
	callBacks.push_back(KeyboardCallBack(this, 'K', "screenshot", "File Name", ScreenshotCallBack));
	callBacks.push_back(KeyboardCallBack(this, 'E', "show boundary", ShowBoundaryEdgesCallBack));
	callBacks.push_back(KeyboardCallBack(this, 'N', "use nearest", NearestSamplingCallBack));


	lightDiffuse[0] = lightDiffuse[1] = lightDiffuse[2] = 1.0f, lightDiffuse[3] = 1.f;
	lightAmbient[0] = lightAmbient[1] = lightAmbient[2] = 0.0f, lightAmbient[3] = 1.f;
	lightSpecular[0] = lightSpecular[1] = lightSpecular[2] = 1.0f, lightSpecular[3] = 1.f;
	lightPosition[3] = 0.f;
	shapeDiffuse[0] = shapeDiffuse[1] = shapeDiffuse[2] = 1.0, shapeDiffuse[3] = 1.0;
	shapeAmbient[0] = shapeAmbient[1] = shapeAmbient[2] = 0.0, shapeAmbient[3] = 1.0;
	shapeSpecular[0] = shapeSpecular[1] = shapeSpecular[2] = 1.0f, shapeSpecular[3] = 1.f;
	shapeSpecularShininess = 128;

	light_direction = vec3(0.f, 0.f, 1.f);
	light_ambient = vec3(0.0f, 0.0f, 0.0f);
	light_diffuse = vec3(1.0f, 1.0f, 1.0f);
	light_specular = vec3(0.0f, 0.0f, 0.0f);
	specular_fallof = 0.f;
}


//(x,y) in [0,screenWidth-1] x [0,screenHeight-1]
//(i,j) in [0,textureImage.width() -1] x [0,textureImage.height() - 1]

Point< float, 2 > TexturedMeshVisualization::screenToImage(int x, int  y) {
	float ic[2] = { float(textureImage.width()) / 2 + xForm.offset[0], float(textureImage.height()) / 2 - xForm.offset[1] };
	float sc[2] = { float(screenWidth) / 2, float(screenHeight) / 2 };
	float scale = imageToScreenScale();
	Point< float, 2 > ip;
	ip[0] = ((float(x) - sc[0]) / scale) + ic[0];
	ip[1] = ((float(y) - sc[1]) / scale) + ic[1];
	return ip;
}

Point2D<float> TexturedMeshVisualization::selectImagePos(int x, int  y) {
	Point2D<float> imagePos;
	if (displayMode == ONE_REGION_DISPLAY) {
		imagePos = Point2D<float>(x / float(screenWidth), y / float(screenHeight));
	}
	else if (displayMode == TWO_REGION_DISPLAY) {
		imagePos = Point2D<float>((x - (screenWidth / 2)) / float(screenWidth / 2), y / float(screenHeight));
	}
	imagePos -= Point2D<float>(0.5, 0.5);

	//Reverse transformation
	float screenScale = std::max<float>(screenWidth, screenHeight);
	imagePos = imagePos / xForm.zoom - Point2D<float>(xForm.offset[0], xForm.offset[1]) / screenScale + Point2D<float>(0.5, 0.5);

	return imagePos;
}

//bool TexturedMeshVisualization::select(int x, int  y, Point3D< float >& out)
//{
//
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	float ar = 1.f;
//	gluPerspective(30, ar, nearPlane, farPlane);
//
//	if (1) out = Point3D< float >(-10, -10, -10);
//	GLfloat projectionMatrix[16];
//	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
//	if (0) printf("GL Projection Matrix\n");
//	if (0) for (int i = 0; i < 4; i++)printf("%f %f %f %f \n", projectionMatrix[i], projectionMatrix[4 + i], projectionMatrix[8 + i], projectionMatrix[12 + i]);
//	float frustrumWidth = 2.f*nearPlane / projectionMatrix[0];
//	float frustrumHeight = 2.f*nearPlane / projectionMatrix[5];
//	if (0) printf("Frustrum Dimensions  %f x %f \n", frustrumWidth, frustrumHeight);
//
//
//
//	int  _screenWidth, _screenHeight;
//	if (displayMode == ONE_REGION_DISPLAY) {
//		_screenWidth = screenWidth;
//		_screenHeight = screenHeight;
//	}
//	else if (displayMode == TWO_REGION_DISPLAY) {
//		_screenWidth = screenWidth / 2;
//		_screenHeight = screenHeight;
//	}
//
//	bool ret = false;
//	Pointer(float) depthBuffer = AllocPointer< float >(sizeof(float)* _screenWidth * _screenHeight);
//	glReadPixels(0, 0, _screenWidth, _screenHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer);
//	int x1 = (int)floor(x), y1 = (int)floor(y), x2 = x1 + 1, y2 = y1 + 1;
//	float dx = x - x1, dy = y - y1;
//	x1 = std::max< int >(0.f, std::min< int >(x1, _screenWidth - 1));
//	y1 = std::max< int >(0.f, std::min< int >(y1, _screenHeight - 1));
//	x2 = std::max< int >(0.f, std::min< int >(x2, _screenWidth - 1));
//	y2 = std::max< int >(0.f, std::min< int >(y2, _screenHeight - 1));
//	float z_depth =
//		depthBuffer[(_screenHeight - 1 - y1)*_screenWidth + x1] * (1.f - dx) * (1.f - dy) +
//		depthBuffer[(_screenHeight - 1 - y1)*_screenWidth + x2] * (dx)* (1.f - dy) +
//		depthBuffer[(_screenHeight - 1 - y2)*_screenWidth + x1] * (1.f - dx) * (dy)+
//		depthBuffer[(_screenHeight - 1 - y2)*_screenWidth + x2] * (dx)* (dy);
//	if (z_depth < 1.f) {
//		float nz = z_depth*2.f - 1.f;
//		float ez = -(2.f*(farPlane*nearPlane) / (farPlane - nearPlane)) / (-(farPlane + nearPlane) / (farPlane - nearPlane) + nz);
//		float ex = (float(x) / float(_screenWidth) - 0.5f)*frustrumWidth*(ez / nearPlane);
//		float ey = -(float(y) / float(_screenHeight) - 0.5f)*frustrumHeight*(ez / nearPlane);
//		if (0) printf("Eye Coordinates %f %f %f \n", ex, ey, ez);
//		Point3D<float> worldCoordinate = camera.position + (camera.right / Point3D<float>::Length(camera.right)) *ex + (camera.up / Point3D<float>::Length(camera.up)) *ey + (camera.forward / Point3D<float>::Length(camera.forward)) *ez;
//		if (0) printf("World Coordinates %f %f %f \n", worldCoordinate[0], worldCoordinate[1], worldCoordinate[2]);
//		out = worldCoordinate;
//		ret = true;
//	}
//	FreePointer(depthBuffer);
//	return ret;
//}

bool TexturedMeshVisualization::select(int x, int  y, Point3D< float >& out) //PERSPECTIVE
{
	if (1) out = Point3D< float >(-10, -10, -10);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMultMatrixf(&camera.projection_matrix[0][0]);

	float frustrumWidth = 2.f*camera.nearest_plane / camera.projection_matrix[0][0];
	float frustrumHeight = 2.f*camera.nearest_plane / camera.projection_matrix[1][1];

	if (0) printf("Frustrum Dimensions  %f x %f \n", frustrumWidth, frustrumHeight);

	bool ret = false;
	Pointer(float) depthBuffer = AllocPointer< float >(sizeof(float)* _screenWidth * _screenHeight);
	glReadPixels(0, 0, _screenWidth, _screenHeight, GL_DEPTH_COMPONENT, GL_FLOAT, depthBuffer);
	int x1 = (int)floor(x), y1 = (int)floor(y), x2 = x1 + 1, y2 = y1 + 1;
	float dx = x - x1, dy = y - y1;
	x1 = std::max< int >(0.f, std::min< int >(x1, _screenWidth - 1));
	y1 = std::max< int >(0.f, std::min< int >(y1, _screenHeight - 1));
	x2 = std::max< int >(0.f, std::min< int >(x2, _screenWidth - 1));
	y2 = std::max< int >(0.f, std::min< int >(y2, _screenHeight - 1));
	float z_depth =
		depthBuffer[(_screenHeight - 1 - y1)*_screenWidth + x1] * (1.f - dx) * (1.f - dy) +
		depthBuffer[(_screenHeight - 1 - y1)*_screenWidth + x2] * (dx)* (1.f - dy) +
		depthBuffer[(_screenHeight - 1 - y2)*_screenWidth + x1] * (1.f - dx) * (dy)+
		depthBuffer[(_screenHeight - 1 - y2)*_screenWidth + x2] * (dx)* (dy);
	if (z_depth < 1.f) {
		float nz = z_depth*2.f - 1.f;
		float ez = -(2.f*(camera.farthest_plane*camera.nearest_plane) / (camera.farthest_plane - camera.nearest_plane)) / (-(camera.farthest_plane + camera.nearest_plane) / (camera.farthest_plane - camera.nearest_plane) + nz);
		float ex = (float(x) / float(_screenWidth) - 0.5f)*frustrumWidth*(ez / camera.nearest_plane);
		float ey = -(float(y) / float(_screenHeight) - 0.5f)*frustrumHeight*(ez / camera.nearest_plane);
		if (0) printf("Eye Coordinates %f %f %f \n", ex, ey, ez);
		vec3 _worldCoordinate = camera.position + camera.right*ex + camera.up*ey + camera.direction*ez;
		if (0) printf("World Coordinates %f %f %f \n", _worldCoordinate[0], _worldCoordinate[1], _worldCoordinate[2]);

		out = Point3D<float>(_worldCoordinate[0], _worldCoordinate[1], _worldCoordinate[2]);
		ret = true;
	}
	FreePointer(depthBuffer);
	return ret;
}