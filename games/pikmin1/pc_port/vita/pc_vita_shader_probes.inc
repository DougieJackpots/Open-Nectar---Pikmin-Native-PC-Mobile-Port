// Included inside pc_gfx_init after source creation. Diagnostic builds only.
#if defined(__vita__) && defined(PIKI_VITA_SHADER_PROBES)
{
    auto probe = [&](const char* name, const std::string& source) {
        printf("[Vita probe] BEGIN %s\n", name);
        fflush(stdout);
        GLuint shader = glCreateShader_ptr(GL_VERTEX_SHADER);
        if (!shader) { printf("[Vita probe] allocation failed\n"); return; }
        const char* text = source.c_str();
        glShaderSource_ptr(shader, 1, &text, nullptr);
        glCompileShader_ptr(shader);
        GLint ok = GL_FALSE;
        glGetShaderiv_ptr(shader, GL_COMPILE_STATUS, &ok);
        char log[2048] = {};
        if (glGetShaderInfoLog_ptr) glGetShaderInfoLog_ptr(shader, sizeof log, nullptr, log);
        printf("[Vita probe] %s: %s %s\n", name, ok == GL_TRUE ? "OK" : "FAIL", log);
        fflush(stdout);
        glDeleteShader_ptr(shader);
    };
    const std::string h = "#version 100\nprecision highp float;\nprecision highp int;\nattribute vec3 aPos;\n";
    probe("minimal", h + "void main(){gl_Position=vec4(aPos,1.0);}\n");
    probe("9 varying declarations", h + "varying vec3 a,b,c,d; varying vec4 e; varying vec2 f,g,j,k;\nvoid main(){a=aPos;b=aPos;c=aPos;d=aPos;e=vec4(aPos,1.0);f=aPos.xy;g=aPos.xy;j=aPos.xy;k=aPos.xy;gl_Position=vec4(aPos,1.0);}\n");
    probe("uniform int array dynamic index", h + "uniform int u[8];\nvoid main(){gl_Position=vec4(aPos,float(u[int(aPos.x)]));}\n");
    probe("2 vec4 palettes dynamic index", h + "uniform vec4 pa[63],pb[63];\nvoid main(){int r=int(aPos.x)*3;gl_Position=vec4(dot(pa[r],vec4(aPos,1.0)),dot(pb[r+1],vec4(aPos,1.0)),aPos.z,1.0);}\n");
    probe("mat4 array dynamic index", h + "uniform mat4 m[8];\nvoid main(){gl_Position=m[int(aPos.y)]*vec4(aPos,1.0);}\n");
    probe("mat3", h + "uniform mat3 nm;\nvoid main(){gl_Position=vec4(nm*aPos,1.0);}\n");
    probe("uniform loop bound", h + "uniform int n;\nvoid main(){float s=0.0;for(int i=0;i<n;i++){s+=aPos.x*float(i);}gl_Position=vec4(aPos,s);}\n");
    probe("constant loop bound plus break", h + "uniform int n;\nvoid main(){float s=0.0;for(int i=0;i<4;i++){if(i>=n)break;s+=aPos.x*float(i);}gl_Position=vec4(aPos,s);}\n");
    probe("array parameter", h + "uniform vec4 lp[4];\nvec3 f(vec4 l[4]){return l[1].xyz;}\nvoid main(){gl_Position=vec4(aPos+f(lp),1.0);}\n");
    probe("nested ternary", h + "uniform int mode;\nvoid main(){gl_Position=(mode==1)?vec4(aPos,1.0):(mode==2)?vec4(aPos.xy,0.0,1.0):vec4(0.0);}\n");
    probe("integer clamp", h + "attribute float aSlot;\nvoid main(){int r=clamp(int(aSlot+0.5),0,20)*3;gl_Position=vec4(aPos,float(r));}\n");
    probe("float clamp", h + "attribute float aSlot;\nvoid main(){int r=int(clamp(floor(aSlot+0.5),0.0,20.0))*3;gl_Position=vec4(aPos,float(r));}\n");
    const std::string real = vShaderSrc ? vShaderSrc : "";
    probe("real submitted vertex", real);
    std::string fixed = real;
    const std::string from = "clamp(int(aMatrixSlot + 0.5), 0, 20)";
    const auto pos = fixed.find(from);
    if (pos != std::string::npos) {
        fixed.replace(pos, from.size(), "int(clamp(floor(aMatrixSlot + 0.5), 0.0, 20.0))");
        probe("real vertex float clamp", fixed);
    }
}
#endif
