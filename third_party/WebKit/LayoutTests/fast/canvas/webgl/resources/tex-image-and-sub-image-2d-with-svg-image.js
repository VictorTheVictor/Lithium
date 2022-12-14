function generateTest(pixelFormat, pixelType, pathToTestRoot, prologue) {
    var wtu = WebGLTestUtils;
    var gl = null;
    var textureLoc = null;
    var successfullyParsed = false;
    var imgCanvas;
    var black = [0, 0, 0];
    var green = [0, 255, 0];
    var image = new Image();
    var imageLoaded = false;
    var poisonImage = new Image();
    var poisonImageLoaded = false;

 var init = function()
 {
     if (window.initNonKhronosFramework) {
         window.initNonKhronosFramework(true);
     }

     description('Verify texImage2D and texSubImage2D code paths taking SVG image elements');

     gl = wtu.create3DContext("example");

     if (!prologue(gl)) {
         finishTest();
         return;
     }

     var program = wtu.setupTexturedQuad(gl);

     gl.clearColor(0,0,0,1);
     gl.clearDepth(1);

     textureLoc = gl.getUniformLocation(program, "tex");

     image.width = 10;
     image.height = 10;
     image.onload = function () {
       imageLoaded = true;
       runTest();
     }
     image.src = pathToTestRoot + "/resources/transparent-green.svg";

     poisonImage.width = 10;
     poisonImage.height = 10;
     poisonImage.onload = function () {
       poisonImageLoaded = true;
       runTest();
     }
     poisonImage.src = pathToTestRoot + "/resources/red-green.svg";

 }

 function runOneIteration(useTexSubImage2D, flipY, topColor, bottomColor)
 {
     debug('Testing ' + (useTexSubImage2D ? 'texSubImage2D' : 'texImage2D') +
           ' with flipY=' + flipY);
     gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
     var texture = gl.createTexture();
     // Bind the texture to texture unit 0.
     gl.bindTexture(gl.TEXTURE_2D, texture);
     // Set up texture parameters.
     gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
     gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
     gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
     gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
     // Set up pixel store parameters.
     gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, flipY);
     gl.pixelStorei(gl.UNPACK_PREMULTIPLY_ALPHA_WEBGL, false);
     gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
     // Upload the image into the texture.
     if (useTexSubImage2D) {
         // Initialize the texture to black first.
         gl.texImage2D(gl.TEXTURE_2D, 0, gl[pixelFormat], image.width, image.height, 0,
                       gl[pixelFormat], gl[pixelType], null);
         gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl[pixelFormat], gl[pixelType], poisonImage);
         gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, gl[pixelFormat], gl[pixelType], image);
     } else {
         gl.texImage2D(gl.TEXTURE_2D, 0, gl[pixelFormat], gl[pixelFormat], gl[pixelType], poisonImage);
         gl.texImage2D(gl.TEXTURE_2D, 0, gl[pixelFormat], gl[pixelFormat], gl[pixelType], image);
     }

     // Point the uniform sampler to texture unit 0.
     gl.uniform1i(textureLoc, 0);
     // Draw the triangles.
     wtu.drawQuad(gl, [0, 255, 0, 255]);
     // Check a few pixels near the top and bottom and make sure they have
     // the right color.
     debug("Checking lower left corner");
     wtu.checkCanvasRect(gl, 4, 4, 2, 2, bottomColor,
                         "shouldBe " + bottomColor);
     debug("Checking upper left corner");
     wtu.checkCanvasRect(gl, 4, gl.canvas.height - 8, 2, 2, topColor,
                         "shouldBe " + topColor);
 }

 function runTest()
 {
     if (!imageLoaded || !poisonImageLoaded)
       return;

     runOneIteration(false, true, black, green);
     runOneIteration(false, false, green, black);
     runOneIteration(true, true, black, green);
     runOneIteration(true, false, green, black);
     finishTest();
 }

 return init;
}
