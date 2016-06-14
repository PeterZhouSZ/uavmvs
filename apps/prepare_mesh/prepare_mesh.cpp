#include <iostream>
#include <vector>

#include "util/arguments.h"
#include "mve/mesh.h"
#include "mve/mesh_io_ply.h"
#include "mve/bundle.h"
#include "mve/bundle_io.h"

typedef unsigned int uint;

struct Arguments {
    std::string in_mesh;
    std::string out_mesh;
};

Arguments parse_args(int argc, char **argv) {
    util::Arguments args;
    args.set_exit_on_error(true);
    args.set_nonopt_maxnum(2);
    args.set_nonopt_minnum(2);
    args.set_usage("Usage: " + std::string(argv[0]) + " [OPTS] IN_MESH OUT_MESH");
    args.set_description("Prepare mesh for ...");
    args.parse(argc, argv);

    Arguments conf;
    conf.in_mesh = args.get_nth_nonopt(0);
    conf.out_mesh = args.get_nth_nonopt(1);
    for (util::ArgResult const* i = args.next_option();
         i != nullptr; i = args.next_option()) {
        switch (i->opt->sopt) {
        default:
            throw std::invalid_argument("Invalid option");
        }
    }

    return conf;
}

#define SAVE_MESH 1
#define APPLY_TRANSFORM 1
int main(int argc, char **argv) {
    Arguments args = parse_args(argc, argv);

    std::cout << "Load mesh: " << std::endl;
    mve::TriangleMesh::Ptr mesh;
    try {
        mesh = mve::geom::load_ply_mesh(args.in_mesh);
    } catch (std::exception& e) {
        std::cerr << "\tCould not load mesh: "<< e.what() << std::endl;
        std::exit(EXIT_FAILURE);
    }
    mesh->ensure_normals(true, false);

    uint const num_faces = mesh->get_faces().size() / 3;
    std::vector<math::Vec3f> & vertices = mesh->get_vertices();
    std::vector<uint> & faces = mesh->get_faces();
    std::vector<math::Vec4f> & colors = mesh->get_vertex_colors();

#if APPLY_TRANSFORM
    math::Matrix4f m(0.0f);
    m[0] =  0.936050f; m[1] = 0.351866f; m[3] = -24.849516f;
    m[4] = -0.351866f; m[5] = 0.936050f; m[7] = -32.347613f;
    m[10] = 1.0f;
    m[15] = 1.0f;

    for (math::Vec3f & vertex : vertices) {
        vertex = m.mult(vertex, 1.0f);
    }
#endif

#if CONVERT_TO_BUNDLE
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    std::vector<mve::Bundle::Feature3D> & features = bundle->get_features();
    features.resize(vertices.size());

    for (std::size_t i = 0; i < vertices.size(); ++i) {
        mve::Bundle::Feature3D & feature = features[i];
        math::Vec3f const & vertex = vertices[i];
        math::Vec4f const & color = colors[i];
        std::copy(vertex.begin(), vertex.end(), feature.pos);
        std::copy(color.begin(), color.end() - 1, feature.color);
    }

    mve::save_mve_bundle(bundle, args.out_mesh);
#endif

#if REMOVE_BOTTOM_FACES
    std::vector<uint> new_faces;
    new_faces.reserve(faces.size());

    math::Vec3f v0 = vertices[faces[9000 * 3 + 0]];
    math::Vec3f v1 = vertices[faces[9000 * 3 + 1]];
    math::Vec3f v2 = vertices[faces[9000 * 3 + 2]];
    math::Vec3f normal = ((v2 - v0).cross(v1 - v0)).normalize();

    for (std::size_t i = 0; i < faces.size(); i += 3) {
        if ((vertices[faces[i + 0]] - v0).dot(normal) < 0.01f &&
            (vertices[faces[i + 1]] - v0).dot(normal) < 0.01f &&
            (vertices[faces[i + 2]] - v0).dot(normal) < 0.01f) continue;
        uint const * face_ptr = faces.data() + i;
        new_faces.insert(new_faces.end(), face_ptr, face_ptr + 3);
    }

    faces.swap(new_faces);
#endif

#if SAVE_MESH
    mve::geom::SavePLYOptions opts;
    opts.write_face_colors = false;
    opts.write_face_normals = true;
    opts.write_vertex_colors = true;
    opts.write_vertex_confidences = false;
    opts.write_vertex_values = false;
    opts.write_vertex_normals = false;
    mve::geom::save_ply_mesh(mesh, args.out_mesh, opts);
#endif
}
