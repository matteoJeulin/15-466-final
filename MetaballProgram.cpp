#include "MetaballProgram.hpp"
#include "SoftBody.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <vector>
#include <string>

Scene::Drawable::Pipeline metaball_program_pipeline;

GLuint cheese_cube_vao = 0;
GLsizei cheese_cube_index_count = 0;

namespace {
	GLuint cheese_cube_vbo = 0;
	GLuint cheese_cube_ebo = 0;

	void init_cheese_cube(GLint position_attr_loc) {
		if (cheese_cube_vao != 0) return; // already created

		struct Vertex { glm::vec3 pos; };
		Vertex verts[] = {
			{{-1.0f, -1.0f,  1.0f}},
			{{ 1.0f, -1.0f,  1.0f}},
			{{ 1.0f,  1.0f,  1.0f}},
			{{-1.0f,  1.0f,  1.0f}},
			{{-1.0f, -1.0f, -1.0f}},
			{{ 1.0f, -1.0f, -1.0f}},
			{{ 1.0f,  1.0f, -1.0f}},
			{{-1.0f,  1.0f, -1.0f}},
		};

		GLushort indices[] = {
			0, 1, 2, 0, 2, 3, // front
			1, 5, 6, 1, 6, 2, // right
			5, 4, 7, 5, 7, 6, // back
			4, 0, 3, 4, 3, 7, // left
			3, 2, 6, 3, 6, 7, // top
			4, 5, 1, 4, 1, 0  // bottom
		};
		cheese_cube_index_count = sizeof(indices) / sizeof(indices[0]);

		glGenVertexArrays(1, &cheese_cube_vao);
		glBindVertexArray(cheese_cube_vao);

		glGenBuffers(1, &cheese_cube_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, cheese_cube_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

		glGenBuffers(1, &cheese_cube_ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cheese_cube_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// enable Position attribute:
		glEnableVertexAttribArray(position_attr_loc);
		glVertexAttribPointer(position_attr_loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glBindVertexArray(0);
	}
}

Load< MetaballProgram > metaball_program(LoadTagEarly, []() -> MetaballProgram const * {
	MetaballProgram *ret = new MetaballProgram();

	// Setup pipeline template:
	metaball_program_pipeline.program = ret->program;
	metaball_program_pipeline.CLIP_FROM_OBJECT_mat4 = ret->CLIP_FROM_OBJECT_mat4;


	// Dummy 1x1 white texture so pipeline has something bound:
	GLuint tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	metaball_program_pipeline.textures[0].texture = tex;
	metaball_program_pipeline.textures[0].target  = GL_TEXTURE_2D;

	return ret;
});

MetaballProgram::MetaballProgram() {
	using std::to_string;

	program = gl_compile_program(
		// Vertex shader:
		"#version 330\n"
		"uniform mat4 CLIP_FROM_OBJECT;\n"
		"in vec4 Position;\n"
		"out vec3 vPosOS; // object-space position on cube surface\n"
		"void main() {\n"
		"    vPosOS = Position.xyz;\n"
		"    gl_Position = CLIP_FROM_OBJECT * Position;\n"
		"}\n"
	,
    // Fragment shader: raymarch metaball surface inside bounding cube
	// 	("#version 330\n"
	// 	 "#define MAX_STEPS 128\n"
	// 	 "#define MAX_DIST  20.0\n"   // max travel inside cube in object units\n"
	// 	 "#define SURF_DIST 0.001\n"
	// 	) +
	// 	std::string("const int MAX_METABALLS = ") + to_string(MaxMetaballs) + ";\n"
	// 	"\n"
	// 	"uniform mat4 CLIP_FROM_OBJECT;\n"
	// 	"uniform float TIME;\n"
	// 	"uniform vec3  EYE; // camera position in object space\n"
	// 	"\n"
	// 	"uniform int  METABALL_COUNT;\n"
	// 	"uniform vec4 METABALLS[MAX_METABALLS]; // xyz: posOS, w: radius\n"
	// 	"uniform float ISO_LEVEL;\n"
	// 	"\n"
	// 	"in vec3 vPosOS;\n"
	// 	"out vec4 fragColor;\n"
	// 	"\n"
	// 	"// Blinn-style metaball field: sum(r / |p - c|^2)\n"
	// 	"float metaball_field(vec3 p) {\n"
	// 	"    float field = 0.0;\n"
	// 	"    for (int i = 0; i < METABALL_COUNT; ++i) {\n"
	// 	"        vec3 c = METABALLS[i].xyz;\n"
	// 	"        float r = METABALLS[i].w;\n"
	// 	"        vec3 d = (p - c) / r;\n"
	// 	"        float dd = dot(d, d) + 1e-5;\n"
	// 	"        field += r / (dd * dd);\n"
	// 	"    }\n"
	// 	"    return field;\n"
	// 	"}\n"
	// 	"\n"
	// 	"// Implicit surface: map(p) == 0 on iso-surface\n"
	// 	"float map_value(vec3 p) {\n"
	// 	"    return ISO_LEVEL - metaball_field(p);\n"
	// 	"}\n"
	// 	"\n"
	// 	"// For stringiness: analyze contributions\n"
	// 	"void field_info(vec3 p, out float totalField, out float maxContrib, out float secondMax) {\n"
	// 	"    totalField = 0.0;\n"
	// 	"    maxContrib = 0.0;\n"
	// 	"    secondMax = 0.0;\n"
	// 	"    for (int i = 0; i < METABALL_COUNT; ++i) {\n"
	// 	"        vec3 c = METABALLS[i].xyz;\n"
	// 	"        float r = METABALLS[i].w;\n"
	// 	"        vec3 d = p - c;\n"
	// 	"        float dd = dot(d, d) + 1e-5;\n"
	// 	"        float w = r / dd;\n"
	// 	"        totalField += w;\n"
	// 	"        if (w > maxContrib) {\n"
	// 	"            secondMax = maxContrib;\n"
	// 	"            maxContrib = w;\n"
	// 	"        } else if (w > secondMax) {\n"
	// 	"            secondMax = w;\n"
	// 	"        }\n"
	// 	"    }\n"
	// 	"}\n"
	// 	"\n"
	// 	"float cheese_bridge_factor(float totalField, float maxContrib, float secondMax) {\n"
	// 	"    if (totalField <= 0.0) return 0.0;\n"
	// 	"    float secondRatio = secondMax / totalField;\n"
	// 	"    return clamp(secondRatio, 0.0, 1.0);\n"
	// 	"}\n"
	// 	"\n"
	// 	"// --- metaball outline helpers ---\n"
	// 	"float metaball_iso_radius(float r_param, float iso_level) {\n"
	// 	"    if (iso_level <= 0.0) return 0.0;\n"
	// 	"    float r2 = r_param * r_param;\n"
	// 	"    float r4 = r2 * r2;\n"
	// 	"    float r5 = r4 * r_param;\n"
	// 	"    return pow(r5 / iso_level, 0.25);\n"
	// 	"}\n"
	// 	"\n"
	// 	"void get_metaball_outline(int i, out vec3 center, out float blobRadius) {\n"
	// 	"    center = METABALLS[i].xyz;\n"
	// 	"    float r_param = METABALLS[i].w;\n"
	// 	"    blobRadius = metaball_iso_radius(r_param, ISO_LEVEL);\n"
	// 	"}\n"
	// 	"\n"
	// 	"void closest_metaball(vec3 p,\n"
	// 	"                      out int idx,\n"
	// 	"                      out vec3 center,\n"
	// 	"                      out float blobRadius,\n"
	// 	"                      out float normDist) {\n"
	// 	"    idx = -1;\n"
	// 	"    center = vec3(0.0);\n"
	// 	"    blobRadius = 1.0;\n"
	// 	"    normDist = 9999.0;\n"
	// 	"\n"
	// 	"    for (int i = 0; i < METABALL_COUNT; ++i) {\n"
	// 	"        vec3 c = METABALLS[i].xyz;\n"
	// 	"        float r_param = METABALLS[i].w;\n"
	// 	"        float R = metaball_iso_radius(r_param, ISO_LEVEL);\n"
	// 	"        if (R <= 0.0) continue;\n"
	// 	"\n"
	// 	"        float d = length(p - c);\n"
	// 	"        float nd = d / R;\n"
	// 	"\n"
	// "        if (nd < normDist) {\n"
	// "            normDist = nd;\n"
	// "            idx = i;\n"
	// "            center = c;\n"
	// "            blobRadius = R;\n"
	// "        }\n"
	// "    }\n"
	// "}\n"
	// "\n"
	// "vec3 get_normal(vec3 p) {\n"
	// "    vec2 e = vec2(0.001, 0.0);\n"
	// "    float dx = map_value(p + vec3(e.x, e.y, e.y)) - map_value(p - vec3(e.x, e.y, e.y));\n"
	// "    float dy = map_value(p + vec3(e.y, e.x, e.y)) - map_value(p - vec3(e.y, e.x, e.y));\n"
	// "    float dz = map_value(p + vec3(e.y, e.y, e.x)) - map_value(p - vec3(e.y, e.y, e.x));\n"
	// "    return normalize(vec3(dx, dy, dz));\n"
	// "}\n"
	// "\n"
	// "// Raymarch starting at t0 (distance from eye to entry point on cube)\n"
	// "bool ray_march(vec3 ro, vec3 rd, float t0, out vec3 pHit) {\n"
	// "    float t = t0;\n"
	// "    for (int i = 0; i < MAX_STEPS; ++i) {\n"
	// "        vec3 p = ro + rd * t;\n"
	// "        float d = map_value(p);\n"
	// "        if (d < SURF_DIST) {\n"
	// "            pHit = p;\n"
	// "            return true;\n"
	// "        }\n"
	// "        if (t > MAX_DIST) break;\n"
	// "        float step = max(d, 0.001);\n"
	// "        t += step;\n"
	// "    }\n"
	// "    return false;\n"
	// "}\n"
	// "\n"
	// "void main() {\n"
	// "    // eye and entry point (object space):\n"
	// "    vec3 ro = EYE;\n"
	// "    vec3 entry = vPosOS; // point on front face of cube\n"
	// "    vec3 rd = normalize(entry - ro);\n"
	// "    float t0 = length(entry - ro);\n"
	// "\n"
	// "    vec3 p;\n"
	// "    bool hit = ray_march(ro, rd, t0, p);\n"
	// "    if (!hit) discard; // no metaball along this ray\n"
	// "\n"
	// "    // Closest metaball info\n"
	// "    int   closestIdx;\n"
	// "    vec3  closestCenter;\n"
	// "    float closestRadius;\n"
	// "    float normDist;\n"
	// "    closest_metaball(p, closestIdx, closestCenter, closestRadius, normDist);\n"
	// "\n"
	// "    // Surface normal from field gradient:\n"
	// "    vec3 N = get_normal(p);\n"
	// "\n"
	// "    // simple light direction in object space:\n"
	// "    vec3 light_dir = normalize(vec3(0.4, 0.8, 0.6));\n"
	// "    float diff = max(dot(N, light_dir), 0.0);\n"
	// "\n"
	// "    // view direction in object space (approx):\n"
	// "    vec3 view_dir = normalize(ro - p);\n"
	// "    vec3 h = normalize(light_dir + view_dir);\n"
	// "    float spec = pow(max(dot(N, h), 0.0), 24.0);\n"
	// "\n"
	// "    // sample field info slightly inside to emphasize volume:\n"
	// "    float totalField, maxContrib, secondMax;\n"
	// "    float totalFieldIn, maxContribIn, secondMaxIn;\n"
	// "\n"
	// "    field_info(p, totalField, maxContrib, secondMax);\n"
	// "    field_info(p - N * 0.03, totalFieldIn, maxContribIn, secondMaxIn);\n"
	// "\n"
	// "    float bridge = cheese_bridge_factor(totalFieldIn, maxContribIn, secondMaxIn);\n"
	// "    float goo = smoothstep(0.15, 0.5, bridge);\n"
	// "    float valley = smoothstep(0.3, 0.8, bridge);\n"
	// "    float thickness = clamp((totalFieldIn - ISO_LEVEL) * 0.6, 0.0, 1.0);\n"
	// "\n"
	// "    // core / rim based on closest metaball distance\n"
	// "    float core = 1.0 - clamp(normDist, 0.0, 1.5);\n"
	// "    float rim  = smoothstep(0.8, 1.2, normDist);\n"
	// "\n"
	// "    vec3 baseCheese   = vec3(1.0, 0.82, 0.35);\n"
	// "    vec3 stringCheese = vec3(1.0, 0.92, 0.65);\n"
	// "    vec3 cheeseColor  = mix(baseCheese, stringCheese, goo);\n"
	// "\n"
	// "    vec3 ambient = vec3(0.10, 0.09, 0.07);\n"
	// "    vec3 lit = ambient + diff * vec3(1.0, 0.95, 0.8) + spec * vec3(0.8);\n"
	// "\n"
	// "    // Darken valleys between blobs, brighten cores and rim\n"
	// "    lit *= (1.0 - 0.25 * valley);\n"
	// "    lit *= (1.0 + 0.2 * core);\n"
	// "    lit += rim * 0.08;\n"
	// "\n"
	// "    vec3 emission = vec3(0.12, 0.09, 0.04) * goo * (0.5 + 0.5 * thickness);\n"
	// "\n"
	// "    vec3 color = cheeseColor * lit + emission;\n"
	// "\n"
	// "    // compute correct depth from p in object space:\n"
	// "    vec4 clip = CLIP_FROM_OBJECT * vec4(p, 1.0);\n"
	// "    float ndc_depth = clip.z / clip.w;        // -1..1\n"
	// "    gl_FragDepth = ndc_depth * 0.5 + 0.5;     // 0..1\n"
	// "\n"
	// "    fragColor = vec4(color, 1.0);\n"
	// "}\n"
		// Fragment shader: raymarch metaball surface inside bounding cube
		("#version 330\n"
		 "#define MAX_STEPS 128\n"
		 "#define MAX_DIST  20.0\n"   // max travel inside cube in object units
		 "#define SURF_DIST 0.005\n"
		) +
		std::string("const int MAX_METABALLS = ") + to_string(MaxMetaballs) + ";\n"
		"\n"
		"uniform mat4 CLIP_FROM_OBJECT;\n"
		"uniform float TIME;\n"
		"uniform vec3  EYE; // camera position in object space\n"
		"\n"
		"uniform int  METABALL_COUNT;\n"
		"uniform vec4 METABALLS[MAX_METABALLS]; // xyz: posOS, w: radius\n"
        "uniform float ISO_LEVEL;\n"
		"\n"
		"in vec3 vPosOS;\n"
		"out vec4 fragColor;\n"
        
		"\n"
		"// Blinn-style metaball field: sum(r / |p - c|^2)\n"
		"float metaball_field(vec3 p) {\n"
		"    float field = 0.0;\n"
		"    for (int i = 0; i < METABALL_COUNT; ++i) {\n"
		"        vec3 c = METABALLS[i].xyz;\n"
		"        float r = METABALLS[i].w;\n"
		"         vec3 d = (p - c) ;\n"
		"        float dd = dot(d, d) + 1e-5;\n"
		"        field += r*r /(dd);\n"
		"    }\n"
		"    return field;\n"
		"}\n"
		"\n"
		"// Implicit surface at field = 1.0 -> map(p) == 0\n"
		"float map_value(vec3 p) {\n"
		"    return ISO_LEVEL  - metaball_field(p);\n"
		"}\n"
		"\n"
		"// For stringiness: analyze contributions\n"
		"void field_info(vec3 p, out float totalField, out float maxContrib, out float secondMax) {\n"
		"    totalField = 0.0;\n"
		"    maxContrib = 0.0;\n"
		"    secondMax = 0.0;\n"
		"    for (int i = 0; i < METABALL_COUNT; ++i) {\n"
		"        vec3 c = METABALLS[i].xyz;\n"
		"        float r = METABALLS[i].w;\n"
		"        vec3 d = p - c;\n"
		"        float dd = dot(d, d) + 1e-5;\n"
		"        float w = r / dd;\n"
		"        totalField += w;\n"
		"        if (w > maxContrib) {\n"
		"            secondMax = maxContrib;\n"
		"            maxContrib = w;\n"
		"        } else if (w > secondMax) {\n"
		"            secondMax = w;\n"
		"        }\n"
		"    }\n"
		"}\n"
		"\n"
		"float cheese_bridge_factor(float totalField, float maxContrib, float secondMax) {\n"
		"    if (totalField <= 0.0) return 0.0;\n"
		"    float secondRatio = secondMax / totalField;\n"
		"    return clamp(secondRatio, 0.0, 1.0);\n"
		"}\n"
		"\n"
		"vec3 get_normal(vec3 p) {\n"
		"    vec2 e = vec2(0.001, 0.0);\n"
		"    float dx = map_value(p + vec3(e.x, e.y, e.y)) - map_value(p - vec3(e.x, e.y, e.y));\n"
		"    float dy = map_value(p + vec3(e.y, e.x, e.y)) - map_value(p - vec3(e.y, e.x, e.y));\n"
		"    float dz = map_value(p + vec3(e.y, e.y, e.x)) - map_value(p - vec3(e.y, e.y, e.x));\n"
		"    return normalize(vec3(dx, dy, dz));\n"
		"}\n"
		"\n"
		"// Raymarch starting at t0 (distance from eye to entry point on cube)\n"
		"bool ray_march(vec3 ro, vec3 rd, float t0, out vec3 pHit) {\n"
		"    float t = t0;\n"
		"    for (int i = 0; i < MAX_STEPS; ++i) {\n"
		"        vec3 p = ro + rd * t;\n"
		"        float d = map_value(p);\n"
		"        if (d < SURF_DIST) {\n"
		"            pHit = p;\n"
		"            return true;\n"
		"        }\n"
		"        if (t > MAX_DIST) break;\n"
		"        // clamp step to avoid getting stuck with tiny or negative values:\n"
		"        float step = max(d, 0.001);\n"
		"        t += step;\n"
		"    }\n"
		"    return false;\n"
		"}\n"
		"\n"
		"void main() {\n"
		"    // eye and entry point (object space):\n"
		"    vec3 ro = EYE;\n"
		"    vec3 entry = vPosOS; // point on front face of cube\n"
		"    vec3 rd = normalize(entry - ro);\n"
		"    float t0 = length(entry - ro);\n"
		"\n"
		"    vec3 p;\n"
		"    bool hit = ray_march(ro, rd, t0, p);\n"
		"    if (!hit) discard; // no metaball along this ray\n"
		"\n"
		"    // Surface normal from field gradient:\n"
		"    vec3 N = get_normal(p);\n"
		"\n"
		"    // simple light direction in object space:\n"
		"    vec3 light_dir = normalize(vec3(0.4, 0.8, 0.6));\n"
		"    float diff = max(dot(N, light_dir), 0.0);\n"
		"\n"
		"    // view direction in object space (approx):\n"
		"    vec3 view_dir = normalize(ro - p);\n"
		"    vec3 h = normalize(light_dir + view_dir);\n"
		"    float spec = pow(max(dot(N, h), 0.0), 24.0);\n"
		"\n"
		"    // sample field info slightly inside to emphasize volume:\n"
		"    float totalField, maxContrib, secondMax;\n"
		"    float totalFieldIn, maxContribIn, secondMaxIn;\n"
		"\n"
		"    field_info(p, totalField, maxContrib, secondMax);\n"
		"    field_info(p - N * 0.03, totalFieldIn, maxContribIn, secondMaxIn);\n"
		"\n"
		"    float bridge = cheese_bridge_factor(totalFieldIn, maxContribIn, secondMaxIn);\n"
		"    float goo = smoothstep(0.15, 0.2, bridge);\n"
		"    float valley = smoothstep(0.2, 0.8, bridge);\n"
		"    float thickness = clamp((totalFieldIn - ISO_LEVEL ) * 0.15, 0.0, 1.0);\n"
		"\n"
		"    vec3 baseCheese   = vec3(1.0, 0.92, 0.65);\n"
		"    vec3 stringCheese = vec3(1.0, 0.72, 0.35);\n"
		"    vec3 cheeseColor  = mix(baseCheese, stringCheese, goo);\n"
		"\n"
        "    // --- procedural turbulence ---\n"
        "    // cheap fake noise from sines in object space\n"
        "    float n = 0.0;\n"
        "    n += sin(p.x * 7.31 + TIME * 1.1);\n"
        "    n += sin(p.y * 5.17 - TIME * 0.7);\n"
        "    n += sin(p.z * 6.41 + TIME * 0.3);\n"
        "    n = n / 3.0;                 // now in [-1, 1]\n"
        "    n = n * 0.5 + 0.5;           // remap to [0, 1]\n"
        "\n"
        "    // stronger turbulence in thin / stringy areas:\n"
        "    float turbMask = goo * thickness;\n"
        "\n"
        "    // blend between two cheese tones using turbulence\n"
        "    vec3 darkerCheese = cheeseColor * vec3(0.85, 0.9, 0.8);\n"
        "    vec3 noisyCheese  = mix(cheeseColor, darkerCheese, n * turbMask);\n"
        "\n"
        "    cheeseColor = noisyCheese;\n"
		"    vec3 ambient = vec3(0.10, 0.09, 0.07);\n"
		"    vec3 lit = ambient + diff * vec3(1.0, 0.95, 0.8) + spec * vec3(0.8);\n"
		"       lit *= (1.0 - 0.3 * valley);\n"
		"\n"
		"    vec3 emission = vec3(0.12, 0.09, 0.04) * goo * (0.5 + 0.5 * thickness);\n"
		"\n"
		"    vec3 color = cheeseColor * lit + emission;\n"
		"\n"
		"    // compute correct depth from p in object space:\n"
		"    vec4 clip = CLIP_FROM_OBJECT * vec4(p, 1.0);\n"
		"    float ndc_depth = clip.z / clip.w;        // -1..1\n"
		"    gl_FragDepth = ndc_depth * 0.5 + 0.5;     // 0..1\n"
		"\n"
		"    fragColor = vec4(color, 1.0);\n"
   

        // "void main() {\n"
        // "    fragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
        // "}\n"
		"}\n"
	);

	// Attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");

	// Uniforms:
	CLIP_FROM_OBJECT_mat4 = glGetUniformLocation(program, "CLIP_FROM_OBJECT");
	TIME_float            = glGetUniformLocation(program, "TIME");
	EYE_vec3              = glGetUniformLocation(program, "EYE");
	METABALL_COUNT_int    = glGetUniformLocation(program, "METABALL_COUNT");
	METABALLS_vec4        = glGetUniformLocation(program, "METABALLS");
    ISO_LEVEL_float = glGetUniformLocation(program, "ISO_LEVEL");

init_cheese_cube(Position_vec4);
	glUseProgram(0);
}

MetaballProgram::~MetaballProgram() {
	glDeleteProgram(program);
	program = 0;
}

void MetaballProgram::set_uniforms(glm::mat4 const &clip_from_object,
                                   float time,
                                   glm::vec3 const &eyeOS) const
{
    glUniformMatrix4fv(CLIP_FROM_OBJECT_mat4, 1, GL_FALSE, glm::value_ptr(clip_from_object));
    glUniform1f(TIME_float, time);
    glUniform3fv(EYE_vec3, 1, glm::value_ptr(eyeOS));
}
