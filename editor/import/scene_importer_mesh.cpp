/*************************************************************************/
/*  scene_importer_mesh.cpp                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "scene_importer_mesh.h"

#include "core/math/random_pcg.h"
#include "core/math/static_raycaster.h"
#include "scene/resources/surface_tool.h"

void EditorSceneImporterMesh::Surface::split_normals(const LocalVector<int> &p_indices, const LocalVector<Vector3> &p_normals) {
	ERR_FAIL_COND(arrays.size() != RS::ARRAY_MAX);

	const PackedVector3Array &vertices = arrays[RS::ARRAY_VERTEX];
	int current_vertex_count = vertices.size();
	int new_vertex_count = p_indices.size();
	int final_vertex_count = current_vertex_count + new_vertex_count;
	const int *indices_ptr = p_indices.ptr();

	for (int i = 0; i < arrays.size(); i++) {
		if (i == RS::ARRAY_INDEX) {
			continue;
		}

		if (arrays[i].get_type() == Variant::NIL) {
			continue;
		}

		switch (arrays[i].get_type()) {
			case Variant::PACKED_VECTOR3_ARRAY: {
				PackedVector3Array data = arrays[i];
				data.resize(final_vertex_count);
				Vector3 *data_ptr = data.ptrw();
				if (i == RS::ARRAY_NORMAL) {
					const Vector3 *normals_ptr = p_normals.ptr();
					memcpy(&data_ptr[current_vertex_count], normals_ptr, sizeof(Vector3) * new_vertex_count);
				} else {
					for (int j = 0; j < new_vertex_count; j++) {
						data_ptr[current_vertex_count + j] = data_ptr[indices_ptr[j]];
					}
				}
				arrays[i] = data;
			} break;
			case Variant::PACKED_VECTOR2_ARRAY: {
				PackedVector2Array data = arrays[i];
				data.resize(final_vertex_count);
				Vector2 *data_ptr = data.ptrw();
				for (int j = 0; j < new_vertex_count; j++) {
					data_ptr[current_vertex_count + j] = data_ptr[indices_ptr[j]];
				}
				arrays[i] = data;
			} break;
			case Variant::PACKED_FLOAT32_ARRAY: {
				PackedFloat32Array data = arrays[i];
				int elements = data.size() / current_vertex_count;
				data.resize(final_vertex_count * elements);
				float *data_ptr = data.ptrw();
				for (int j = 0; j < new_vertex_count; j++) {
					memcpy(&data_ptr[(current_vertex_count + j) * elements], &data_ptr[indices_ptr[j] * elements], sizeof(float) * elements);
				}
				arrays[i] = data;
			} break;
			case Variant::PACKED_INT32_ARRAY: {
				PackedInt32Array data = arrays[i];
				int elements = data.size() / current_vertex_count;
				data.resize(final_vertex_count * elements);
				int32_t *data_ptr = data.ptrw();
				for (int j = 0; j < new_vertex_count; j++) {
					memcpy(&data_ptr[(current_vertex_count + j) * elements], &data_ptr[indices_ptr[j] * elements], sizeof(int32_t) * elements);
				}
				arrays[i] = data;
			} break;
			case Variant::PACKED_BYTE_ARRAY: {
				PackedByteArray data = arrays[i];
				int elements = data.size() / current_vertex_count;
				data.resize(final_vertex_count * elements);
				uint8_t *data_ptr = data.ptrw();
				for (int j = 0; j < new_vertex_count; j++) {
					memcpy(&data_ptr[(current_vertex_count + j) * elements], &data_ptr[indices_ptr[j] * elements], sizeof(uint8_t) * elements);
				}
				arrays[i] = data;
			} break;
			case Variant::PACKED_COLOR_ARRAY: {
				PackedColorArray data = arrays[i];
				data.resize(final_vertex_count);
				Color *data_ptr = data.ptrw();
				for (int j = 0; j < new_vertex_count; j++) {
					data_ptr[current_vertex_count + j] = data_ptr[indices_ptr[j]];
				}
			} break;
			default: {
				ERR_FAIL_MSG("Uhandled array type");
			} break;
		}
	}
}

void EditorSceneImporterMesh::add_blend_shape(const String &p_name) {
	ERR_FAIL_COND(surfaces.size() > 0);
	blend_shapes.push_back(p_name);
}

int EditorSceneImporterMesh::get_blend_shape_count() const {
	return blend_shapes.size();
}

String EditorSceneImporterMesh::get_blend_shape_name(int p_blend_shape) const {
	ERR_FAIL_INDEX_V(p_blend_shape, blend_shapes.size(), String());
	return blend_shapes[p_blend_shape];
}

void EditorSceneImporterMesh::set_blend_shape_mode(Mesh::BlendShapeMode p_blend_shape_mode) {
	blend_shape_mode = p_blend_shape_mode;
}

Mesh::BlendShapeMode EditorSceneImporterMesh::get_blend_shape_mode() const {
	return blend_shape_mode;
}

void EditorSceneImporterMesh::add_surface(Mesh::PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes, const Dictionary &p_lods, const Ref<Material> &p_material, const String &p_name) {
	ERR_FAIL_COND(p_blend_shapes.size() != blend_shapes.size());
	ERR_FAIL_COND(p_arrays.size() != Mesh::ARRAY_MAX);
	Surface s;
	s.primitive = p_primitive;
	s.arrays = p_arrays;
	s.name = p_name;

	Vector<Vector3> vertex_array = p_arrays[Mesh::ARRAY_VERTEX];
	int vertex_count = vertex_array.size();
	ERR_FAIL_COND(vertex_count == 0);

	for (int i = 0; i < blend_shapes.size(); i++) {
		Array bsdata = p_blend_shapes[i];
		ERR_FAIL_COND(bsdata.size() != Mesh::ARRAY_MAX);
		Vector<Vector3> vertex_data = bsdata[Mesh::ARRAY_VERTEX];
		ERR_FAIL_COND(vertex_data.size() != vertex_count);
		Surface::BlendShape bs;
		bs.arrays = bsdata;
		s.blend_shape_data.push_back(bs);
	}

	List<Variant> lods;
	p_lods.get_key_list(&lods);
	for (const Variant &E : lods) {
		ERR_CONTINUE(!E.is_num());
		Surface::LOD lod;
		lod.distance = E;
		lod.indices = p_lods[E];
		ERR_CONTINUE(lod.indices.size() == 0);
		s.lods.push_back(lod);
	}

	s.material = p_material;

	surfaces.push_back(s);
	mesh.unref();
}

int EditorSceneImporterMesh::get_surface_count() const {
	return surfaces.size();
}

Mesh::PrimitiveType EditorSceneImporterMesh::get_surface_primitive_type(int p_surface) {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Mesh::PRIMITIVE_MAX);
	return surfaces[p_surface].primitive;
}
Array EditorSceneImporterMesh::get_surface_arrays(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Array());
	return surfaces[p_surface].arrays;
}
String EditorSceneImporterMesh::get_surface_name(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), String());
	return surfaces[p_surface].name;
}
void EditorSceneImporterMesh::set_surface_name(int p_surface, const String &p_name) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].name = p_name;
	mesh.unref();
}

Array EditorSceneImporterMesh::get_surface_blend_shape_arrays(int p_surface, int p_blend_shape) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Array());
	ERR_FAIL_INDEX_V(p_blend_shape, surfaces[p_surface].blend_shape_data.size(), Array());
	return surfaces[p_surface].blend_shape_data[p_blend_shape].arrays;
}
int EditorSceneImporterMesh::get_surface_lod_count(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	return surfaces[p_surface].lods.size();
}
Vector<int> EditorSceneImporterMesh::get_surface_lod_indices(int p_surface, int p_lod) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Vector<int>());
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), Vector<int>());

	return surfaces[p_surface].lods[p_lod].indices;
}

float EditorSceneImporterMesh::get_surface_lod_size(int p_surface, int p_lod) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), 0);
	ERR_FAIL_INDEX_V(p_lod, surfaces[p_surface].lods.size(), 0);
	return surfaces[p_surface].lods[p_lod].distance;
}

Ref<Material> EditorSceneImporterMesh::get_surface_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, surfaces.size(), Ref<Material>());
	return surfaces[p_surface].material;
}

void EditorSceneImporterMesh::set_surface_material(int p_surface, const Ref<Material> &p_material) {
	ERR_FAIL_INDEX(p_surface, surfaces.size());
	surfaces.write[p_surface].material = p_material;
	mesh.unref();
}

void EditorSceneImporterMesh::generate_lods(float p_normal_split_angle) {
	if (!SurfaceTool::simplify_scale_func) {
		return;
	}
	if (!SurfaceTool::simplify_with_attrib_func) {
		return;
	}
	if (!SurfaceTool::optimize_vertex_cache_func) {
		return;
	}

	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].primitive != Mesh::PRIMITIVE_TRIANGLES) {
			continue;
		}

		surfaces.write[i].lods.clear();
		Vector<Vector3> vertices = surfaces[i].arrays[RS::ARRAY_VERTEX];
		PackedInt32Array indices = surfaces[i].arrays[RS::ARRAY_INDEX];
		Vector<Vector3> normals = surfaces[i].arrays[RS::ARRAY_NORMAL];

		if (indices.size() == 0) {
			continue; //no lods if no indices
		}

		unsigned int index_count = indices.size();
		unsigned int vertex_count = vertices.size();
		const Vector3 *vertices_ptr = vertices.ptr();

		LocalVector<float> attributes;
		LocalVector<float> normal_weights;
		int attribute_count = 0;
		if (normals.size()) {
			attribute_count = 3;
			normal_weights.resize(vertex_count);
			for (unsigned int j = 0; j < vertex_count; j++) {
				normal_weights[j] = 5.0; // Give some weight to normal preservation, may be worth exposing as an import setting
			}
		}

		const float max_mesh_error = FLT_MAX; // We don't want to limit by error, just by index target
		float normal_threshold = Math::cos(Math::deg2rad(p_normal_split_angle));
		float scale = SurfaceTool::simplify_scale_func((const float *)vertices_ptr, vertex_count, sizeof(Vector3));
		float mesh_error = 0.0f;

		unsigned int index_target = 12; // Start with the smallest target, 4 triangles
		unsigned int last_index_count = 0;

		const int32_t *indices_ptr = indices.ptr();
		const Vector3 *normals_ptr = normals.ptr();

		int split_vertex_count = vertex_count;
		LocalVector<Vector3> split_vertex_normals;
		LocalVector<int> split_vertex_indices;
		split_vertex_normals.reserve(index_count / 3);
		split_vertex_indices.reserve(index_count / 3);

		RandomPCG pcg;
		pcg.seed(123456789); // Keep seed constant across imports

		LocalVector<Vector3> face_normals;
		Vector3 *face_normals_ptr = nullptr;

		Ref<StaticRaycaster> raycaster = StaticRaycaster::create();
		if (raycaster.is_valid()) {
			raycaster->add_mesh(vertices, indices, 0);
			raycaster->commit();

			if (normals.is_empty()) {
				face_normals.resize(index_count / 3);
				face_normals_ptr = face_normals.ptr();
				for (unsigned int j = 0; j < index_count / 3; j++) {
					const Vector3 &v0 = vertices_ptr[indices_ptr[j * 3 + 0]];
					const Vector3 &v1 = vertices_ptr[indices_ptr[j * 3 + 1]];
					const Vector3 &v2 = vertices_ptr[indices_ptr[j * 3 + 2]];
					face_normals_ptr[j] = vec3_cross(v0 - v2, v0 - v1).normalized();
				}
			}
		}

		while (index_target < index_count) {
			PackedInt32Array new_indices;
			new_indices.resize(index_count);

			size_t new_index_count = SurfaceTool::simplify_with_attrib_func((unsigned int *)new_indices.ptrw(), (const uint32_t *)indices.ptr(), index_count, (const float *)vertices_ptr, vertex_count, sizeof(Vector3), index_target, max_mesh_error, &mesh_error, (float *)normals.ptr(), normal_weights.ptr(), attribute_count);

			if (new_index_count < last_index_count * 2) {
				index_target = MAX(last_index_count, new_index_count) * 1.5f;
				continue;
			}

			if (new_index_count > index_count / 2 || mesh_error == 0.0f) {
				break;
			}

			new_indices.resize(new_index_count);

			if (raycaster.is_valid()) {
				float error_factor = 1.0f / (scale * MAX(mesh_error, 0.15));
				const float ray_bias = 0.01;
				float ray_length = ray_bias + mesh_error * scale * 1.8f;

				Vector<StaticRaycaster::Ray> rays;
				LocalVector<Vector2> ray_uvs;

				int32_t *new_indices_ptr = new_indices.ptrw();

				int current_ray_count = 0;
				for (unsigned int j = 0; j < new_index_count; j += 3) {
					const Vector3 &v0 = vertices_ptr[new_indices_ptr[j + 0]];
					const Vector3 &v1 = vertices_ptr[new_indices_ptr[j + 1]];
					const Vector3 &v2 = vertices_ptr[new_indices_ptr[j + 2]];
					Vector3 face_normal = vec3_cross(v0 - v2, v0 - v1);
					float face_area = face_normal.length(); // Actually twice the face area, since it's the same error_factor on all faces, we don't care

					Vector3 dir = face_normal / face_area;
					int ray_count = CLAMP(5.0 * face_area * error_factor, 6, 64);

					rays.resize(current_ray_count + ray_count);
					StaticRaycaster::Ray *rays_ptr = rays.ptrw();

					ray_uvs.resize(current_ray_count + ray_count);
					Vector2 *ray_uvs_ptr = ray_uvs.ptr();

					for (int k = 0; k < ray_count; k++) {
						float u = pcg.randf();
						float v = pcg.randf();

						if (u + v >= 1.0f) {
							u = 1.0f - u;
							v = 1.0f - v;
						}

						u = 0.9f * u + 0.05f / 3.0f; // Give barycentric coordinates some padding, we don't want to sample right on the edge
						v = 0.9f * v + 0.05f / 3.0f; // v = (v - one_third) * 0.95f + one_third;
						float w = 1.0f - u - v;

						Vector3 org = v0 * w + v1 * u + v2 * v;
						org -= dir * ray_bias;
						rays_ptr[current_ray_count + k] = StaticRaycaster::Ray(org, dir, 0.0f, ray_length);
						rays_ptr[current_ray_count + k].id = j / 3;
						ray_uvs_ptr[current_ray_count + k] = Vector2(u, v);
					}

					current_ray_count += ray_count;
				}

				raycaster->intersect(rays);

				LocalVector<Vector3> ray_normals;
				LocalVector<float> ray_normal_weights;

				ray_normals.resize(new_index_count);
				ray_normal_weights.resize(new_index_count);

				for (unsigned int j = 0; j < new_index_count; j++) {
					ray_normal_weights[j] = 0.0f;
				}

				const StaticRaycaster::Ray *rp = rays.ptr();
				for (int j = 0; j < rays.size(); j++) {
					if (rp[j].geomID != 0) { // Ray missed
						continue;
					}

					if (rp[j].normal.normalized().dot(rp[j].dir) > 0.0f) { // Hit a back face.
						continue;
					}

					const float &u = rp[j].u;
					const float &v = rp[j].v;
					const float w = 1.0f - u - v;

					const unsigned int &hit_tri_id = rp[j].primID;
					const unsigned int &orig_tri_id = rp[j].id;

					Vector3 normal;
					if (normals.is_empty()) {
						normal = face_normals_ptr[hit_tri_id];
					} else {
						const Vector3 &n0 = normals_ptr[indices_ptr[hit_tri_id * 3 + 0]];
						const Vector3 &n1 = normals_ptr[indices_ptr[hit_tri_id * 3 + 1]];
						const Vector3 &n2 = normals_ptr[indices_ptr[hit_tri_id * 3 + 2]];
						normal = n0 * w + n1 * u + n2 * v;
					}

					Vector2 orig_uv = ray_uvs[j];
					float orig_bary[3] = { 1.0f - orig_uv.x - orig_uv.y, orig_uv.x, orig_uv.y };
					for (int k = 0; k < 3; k++) {
						int idx = orig_tri_id * 3 + k;
						float weight = orig_bary[k] * orig_bary[k];
						ray_normals[idx] += normal * weight;
						ray_normal_weights[idx] += weight;
					}
				}

				for (unsigned int j = 0; j < new_index_count; j++) {
					if (ray_normal_weights[j] < 1.0f) { // Not enough weight, rays_normal would be just a bad guess
						continue;
					}

					Vector3 rays_normal = ray_normals[j] / ray_normal_weights[j];
					const int &idx = new_indices_ptr[j];
					if (rays_normal.normalized().dot(normals[idx].normalized()) < normal_threshold) {
						split_vertex_indices.push_back(idx);
						split_vertex_normals.push_back(rays_normal);
						new_indices_ptr[j] = split_vertex_count++;
					}
				}
			}

			Surface::LOD lod;
			lod.distance = mesh_error * scale;
			lod.indices = new_indices;
			//print_line(vformat("Lod %d begin with %d triangles and shoot for %d triangles. Got %d triangles. Lod screen ratio %s.", surfaces.write[i].lods.size(), index_count / 3, index_target / 3, new_index_count / 3, vformat("%f (%f relative error)", lod.distance, mesh_error)));
			surfaces.write[i].lods.push_back(lod);
			index_target *= 2;
			last_index_count = new_index_count;
		}
		//int new_verts_count = split_vertex_count - vertex_count;
		//print_line(vformat("%d new vertices, %f per cent increase.", new_verts_count, float(new_verts_count) / index_count * 100.0f));
		surfaces.write[i].split_normals(split_vertex_indices, split_vertex_normals);
		surfaces.write[i].lods.sort_custom<Surface::LODComparator>();

		for (int j = 0; j < surfaces.write[i].lods.size(); j++) {
			Surface::LOD &lod = surfaces.write[i].lods.write[j];
			unsigned int *lod_indices_ptr = (unsigned int *)lod.indices.ptrw();
			SurfaceTool::optimize_vertex_cache_func(lod_indices_ptr, lod_indices_ptr, lod.indices.size(), split_vertex_count);
		}
	}
}

bool EditorSceneImporterMesh::has_mesh() const {
	return mesh.is_valid();
}

Ref<ArrayMesh> EditorSceneImporterMesh::get_mesh(const Ref<ArrayMesh> &p_base) {
	ERR_FAIL_COND_V(surfaces.size() == 0, Ref<ArrayMesh>());

	if (mesh.is_null()) {
		if (p_base.is_valid()) {
			mesh = p_base;
		}
		if (mesh.is_null()) {
			mesh.instantiate();
		}
		mesh->set_name(get_name());
		if (has_meta("import_id")) {
			mesh->set_meta("import_id", get_meta("import_id"));
		}
		for (int i = 0; i < blend_shapes.size(); i++) {
			mesh->add_blend_shape(blend_shapes[i]);
		}
		mesh->set_blend_shape_mode(blend_shape_mode);
		for (int i = 0; i < surfaces.size(); i++) {
			Array bs_data;
			if (surfaces[i].blend_shape_data.size()) {
				for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
					bs_data.push_back(surfaces[i].blend_shape_data[j].arrays);
				}
			}
			Dictionary lods;
			if (surfaces[i].lods.size()) {
				for (int j = 0; j < surfaces[i].lods.size(); j++) {
					lods[surfaces[i].lods[j].distance] = surfaces[i].lods[j].indices;
				}
			}

			mesh->add_surface_from_arrays(surfaces[i].primitive, surfaces[i].arrays, bs_data, lods);
			if (surfaces[i].material.is_valid()) {
				mesh->surface_set_material(mesh->get_surface_count() - 1, surfaces[i].material);
			}
			if (surfaces[i].name != String()) {
				mesh->surface_set_name(mesh->get_surface_count() - 1, surfaces[i].name);
			}
		}

		mesh->set_lightmap_size_hint(lightmap_size_hint);

		if (shadow_mesh.is_valid()) {
			Ref<ArrayMesh> shadow = shadow_mesh->get_mesh();
			mesh->set_shadow_mesh(shadow);
		}
	}

	return mesh;
}

void EditorSceneImporterMesh::clear() {
	surfaces.clear();
	blend_shapes.clear();
	mesh.unref();
}

void EditorSceneImporterMesh::create_shadow_mesh() {
	if (shadow_mesh.is_valid()) {
		shadow_mesh.unref();
	}

	//no shadow mesh for blendshapes
	if (blend_shapes.size() > 0) {
		return;
	}
	//no shadow mesh for skeletons
	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].arrays[RS::ARRAY_BONES].get_type() != Variant::NIL) {
			return;
		}
		if (surfaces[i].arrays[RS::ARRAY_WEIGHTS].get_type() != Variant::NIL) {
			return;
		}
	}

	shadow_mesh.instantiate();

	for (int i = 0; i < surfaces.size(); i++) {
		LocalVector<int> vertex_remap;
		Vector<Vector3> new_vertices;
		Vector<Vector3> vertices = surfaces[i].arrays[RS::ARRAY_VERTEX];
		int vertex_count = vertices.size();
		{
			Map<Vector3, int> unique_vertices;
			const Vector3 *vptr = vertices.ptr();
			for (int j = 0; j < vertex_count; j++) {
				const Vector3 &v = vptr[j];

				Map<Vector3, int>::Element *E = unique_vertices.find(v);

				if (E) {
					vertex_remap.push_back(E->get());
				} else {
					int vcount = unique_vertices.size();
					unique_vertices[v] = vcount;
					vertex_remap.push_back(vcount);
					new_vertices.push_back(v);
				}
			}
		}

		Array new_surface;
		new_surface.resize(RS::ARRAY_MAX);
		Dictionary lods;

		//		print_line("original vertex count: " + itos(vertices.size()) + " new vertex count: " + itos(new_vertices.size()));

		new_surface[RS::ARRAY_VERTEX] = new_vertices;

		Vector<int> indices = surfaces[i].arrays[RS::ARRAY_INDEX];
		if (indices.size()) {
			int index_count = indices.size();
			const int *index_rptr = indices.ptr();
			Vector<int> new_indices;
			new_indices.resize(indices.size());
			int *index_wptr = new_indices.ptrw();

			for (int j = 0; j < index_count; j++) {
				int index = index_rptr[j];
				ERR_FAIL_INDEX(index, vertex_count);
				index_wptr[j] = vertex_remap[index];
			}

			new_surface[RS::ARRAY_INDEX] = new_indices;

			// Make sure the same LODs as the full version are used.
			// This makes it more coherent between rendered model and its shadows.
			for (int j = 0; j < surfaces[i].lods.size(); j++) {
				indices = surfaces[i].lods[j].indices;

				index_count = indices.size();
				index_rptr = indices.ptr();
				new_indices.resize(indices.size());
				index_wptr = new_indices.ptrw();

				for (int k = 0; k < index_count; k++) {
					int index = index_rptr[k];
					ERR_FAIL_INDEX(index, vertex_count);
					index_wptr[k] = vertex_remap[index];
				}

				lods[surfaces[i].lods[j].distance] = new_indices;
			}
		}

		shadow_mesh->add_surface(surfaces[i].primitive, new_surface, Array(), lods, Ref<Material>(), surfaces[i].name);
	}
}

Ref<EditorSceneImporterMesh> EditorSceneImporterMesh::get_shadow_mesh() const {
	return shadow_mesh;
}

void EditorSceneImporterMesh::_set_data(const Dictionary &p_data) {
	clear();
	if (p_data.has("blend_shape_names")) {
		blend_shapes = p_data["blend_shape_names"];
	}
	if (p_data.has("surfaces")) {
		Array surface_arr = p_data["surfaces"];
		for (int i = 0; i < surface_arr.size(); i++) {
			Dictionary s = surface_arr[i];
			ERR_CONTINUE(!s.has("primitive"));
			ERR_CONTINUE(!s.has("arrays"));
			Mesh::PrimitiveType prim = Mesh::PrimitiveType(int(s["primitive"]));
			ERR_CONTINUE(prim >= Mesh::PRIMITIVE_MAX);
			Array arr = s["arrays"];
			Dictionary lods;
			String name;
			if (s.has("name")) {
				name = s["name"];
			}
			if (s.has("lods")) {
				lods = s["lods"];
			}
			Array b_shapes;
			if (s.has("b_shapes")) {
				b_shapes = s["b_shapes"];
			}
			Ref<Material> material;
			if (s.has("material")) {
				material = s["material"];
			}
			add_surface(prim, arr, b_shapes, lods, material, name);
		}
	}
}
Dictionary EditorSceneImporterMesh::_get_data() const {
	Dictionary data;
	if (blend_shapes.size()) {
		data["blend_shape_names"] = blend_shapes;
	}
	Array surface_arr;
	for (int i = 0; i < surfaces.size(); i++) {
		Dictionary d;
		d["primitive"] = surfaces[i].primitive;
		d["arrays"] = surfaces[i].arrays;
		if (surfaces[i].blend_shape_data.size()) {
			Array bs_data;
			for (int j = 0; j < surfaces[i].blend_shape_data.size(); j++) {
				bs_data.push_back(surfaces[i].blend_shape_data[j].arrays);
			}
			d["blend_shapes"] = bs_data;
		}
		if (surfaces[i].lods.size()) {
			Dictionary lods;
			for (int j = 0; j < surfaces[i].lods.size(); j++) {
				lods[surfaces[i].lods[j].distance] = surfaces[i].lods[j].indices;
			}
			d["lods"] = lods;
		}

		if (surfaces[i].material.is_valid()) {
			d["material"] = surfaces[i].material;
		}

		if (surfaces[i].name != String()) {
			d["name"] = surfaces[i].name;
		}

		surface_arr.push_back(d);
	}
	data["surfaces"] = surface_arr;
	return data;
}

Vector<Face3> EditorSceneImporterMesh::get_faces() const {
	Vector<Face3> faces;
	for (int i = 0; i < surfaces.size(); i++) {
		if (surfaces[i].primitive == Mesh::PRIMITIVE_TRIANGLES) {
			Vector<Vector3> vertices = surfaces[i].arrays[Mesh::ARRAY_VERTEX];
			Vector<int> indices = surfaces[i].arrays[Mesh::ARRAY_INDEX];
			if (indices.size()) {
				for (int j = 0; j < indices.size(); j += 3) {
					Face3 f;
					f.vertex[0] = vertices[indices[j + 0]];
					f.vertex[1] = vertices[indices[j + 1]];
					f.vertex[2] = vertices[indices[j + 2]];
					faces.push_back(f);
				}
			} else {
				for (int j = 0; j < vertices.size(); j += 3) {
					Face3 f;
					f.vertex[0] = vertices[j + 0];
					f.vertex[1] = vertices[j + 1];
					f.vertex[2] = vertices[j + 2];
					faces.push_back(f);
				}
			}
		}
	}

	return faces;
}

Vector<Ref<Shape3D>> EditorSceneImporterMesh::convex_decompose() const {
	ERR_FAIL_COND_V(!Mesh::convex_composition_function, Vector<Ref<Shape3D>>());

	const Vector<Face3> faces = get_faces();

	Vector<Vector<Face3>> decomposed = Mesh::convex_composition_function(faces, -1);

	Vector<Ref<Shape3D>> ret;

	for (int i = 0; i < decomposed.size(); i++) {
		Set<Vector3> points;
		for (int j = 0; j < decomposed[i].size(); j++) {
			points.insert(decomposed[i][j].vertex[0]);
			points.insert(decomposed[i][j].vertex[1]);
			points.insert(decomposed[i][j].vertex[2]);
		}

		Vector<Vector3> convex_points;
		convex_points.resize(points.size());
		{
			Vector3 *w = convex_points.ptrw();
			int idx = 0;
			for (Set<Vector3>::Element *E = points.front(); E; E = E->next()) {
				w[idx++] = E->get();
			}
		}

		Ref<ConvexPolygonShape3D> shape;
		shape.instantiate();
		shape->set_points(convex_points);
		ret.push_back(shape);
	}

	return ret;
}

Ref<Shape3D> EditorSceneImporterMesh::create_trimesh_shape() const {
	Vector<Face3> faces = get_faces();
	if (faces.size() == 0) {
		return Ref<Shape3D>();
	}

	Vector<Vector3> face_points;
	face_points.resize(faces.size() * 3);

	for (int i = 0; i < face_points.size(); i += 3) {
		Face3 f = faces.get(i / 3);
		face_points.set(i, f.vertex[0]);
		face_points.set(i + 1, f.vertex[1]);
		face_points.set(i + 2, f.vertex[2]);
	}

	Ref<ConcavePolygonShape3D> shape = memnew(ConcavePolygonShape3D);
	shape->set_faces(face_points);
	return shape;
}

Ref<NavigationMesh> EditorSceneImporterMesh::create_navigation_mesh() {
	Vector<Face3> faces = get_faces();
	if (faces.size() == 0) {
		return Ref<NavigationMesh>();
	}

	Map<Vector3, int> unique_vertices;
	LocalVector<int> face_indices;

	for (int i = 0; i < faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			Vector3 v = faces[i].vertex[j];
			int idx;
			if (unique_vertices.has(v)) {
				idx = unique_vertices[v];
			} else {
				idx = unique_vertices.size();
				unique_vertices[v] = idx;
			}
			face_indices.push_back(idx);
		}
	}

	Vector<Vector3> vertices;
	vertices.resize(unique_vertices.size());
	for (Map<Vector3, int>::Element *E = unique_vertices.front(); E; E = E->next()) {
		vertices.write[E->get()] = E->key();
	}

	Ref<NavigationMesh> nm;
	nm.instantiate();
	nm->set_vertices(vertices);

	Vector<int> v3;
	v3.resize(3);
	for (uint32_t i = 0; i < face_indices.size(); i += 3) {
		v3.write[0] = face_indices[i + 0];
		v3.write[1] = face_indices[i + 1];
		v3.write[2] = face_indices[i + 2];
		nm->add_polygon(v3);
	}

	return nm;
}

extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, int p_index_count, const uint8_t *p_cache_data, bool *r_use_cache, uint8_t **r_mesh_cache, int *r_mesh_cache_size, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y);

struct EditorSceneImporterMeshLightmapSurface {
	Ref<Material> material;
	LocalVector<SurfaceTool::Vertex> vertices;
	Mesh::PrimitiveType primitive = Mesh::PrimitiveType::PRIMITIVE_MAX;
	uint32_t format = 0;
	String name;
};

Error EditorSceneImporterMesh::lightmap_unwrap_cached(const Transform3D &p_base_transform, float p_texel_size, const Vector<uint8_t> &p_src_cache, Vector<uint8_t> &r_dst_cache) {
	ERR_FAIL_COND_V(!array_mesh_lightmap_unwrap_callback, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(blend_shapes.size() != 0, ERR_UNAVAILABLE, "Can't unwrap mesh with blend shapes.");

	LocalVector<float> vertices;
	LocalVector<float> normals;
	LocalVector<int> indices;
	LocalVector<float> uv;
	LocalVector<Pair<int, int>> uv_indices;

	Vector<EditorSceneImporterMeshLightmapSurface> lightmap_surfaces;

	// Keep only the scale
	Basis basis = p_base_transform.get_basis();
	Vector3 scale = Vector3(basis.get_axis(0).length(), basis.get_axis(1).length(), basis.get_axis(2).length());

	Transform3D transform;
	transform.scale(scale);

	Basis normal_basis = transform.basis.inverse().transposed();

	for (int i = 0; i < get_surface_count(); i++) {
		EditorSceneImporterMeshLightmapSurface s;
		s.primitive = get_surface_primitive_type(i);

		ERR_FAIL_COND_V_MSG(s.primitive != Mesh::PRIMITIVE_TRIANGLES, ERR_UNAVAILABLE, "Only triangles are supported for lightmap unwrap.");
		Array arrays = get_surface_arrays(i);
		s.material = get_surface_material(i);
		s.name = get_surface_name(i);

		SurfaceTool::create_vertex_array_from_triangle_arrays(arrays, s.vertices, &s.format);

		PackedVector3Array rvertices = arrays[Mesh::ARRAY_VERTEX];
		int vc = rvertices.size();

		PackedVector3Array rnormals = arrays[Mesh::ARRAY_NORMAL];

		int vertex_ofs = vertices.size() / 3;

		vertices.resize((vertex_ofs + vc) * 3);
		normals.resize((vertex_ofs + vc) * 3);
		uv_indices.resize(vertex_ofs + vc);

		for (int j = 0; j < vc; j++) {
			Vector3 v = transform.xform(rvertices[j]);
			Vector3 n = normal_basis.xform(rnormals[j]).normalized();

			vertices[(j + vertex_ofs) * 3 + 0] = v.x;
			vertices[(j + vertex_ofs) * 3 + 1] = v.y;
			vertices[(j + vertex_ofs) * 3 + 2] = v.z;
			normals[(j + vertex_ofs) * 3 + 0] = n.x;
			normals[(j + vertex_ofs) * 3 + 1] = n.y;
			normals[(j + vertex_ofs) * 3 + 2] = n.z;
			uv_indices[j + vertex_ofs] = Pair<int, int>(i, j);
		}

		PackedInt32Array rindices = arrays[Mesh::ARRAY_INDEX];
		int ic = rindices.size();

		float eps = 1.19209290e-7F; // Taken from xatlas.h
		if (ic == 0) {
			for (int j = 0; j < vc / 3; j++) {
				Vector3 p0 = transform.xform(rvertices[j * 3 + 0]);
				Vector3 p1 = transform.xform(rvertices[j * 3 + 1]);
				Vector3 p2 = transform.xform(rvertices[j * 3 + 2]);

				if ((p0 - p1).length_squared() < eps || (p1 - p2).length_squared() < eps || (p2 - p0).length_squared() < eps) {
					continue;
				}

				indices.push_back(vertex_ofs + j * 3 + 0);
				indices.push_back(vertex_ofs + j * 3 + 1);
				indices.push_back(vertex_ofs + j * 3 + 2);
			}

		} else {
			for (int j = 0; j < ic / 3; j++) {
				Vector3 p0 = transform.xform(rvertices[rindices[j * 3 + 0]]);
				Vector3 p1 = transform.xform(rvertices[rindices[j * 3 + 1]]);
				Vector3 p2 = transform.xform(rvertices[rindices[j * 3 + 2]]);

				if ((p0 - p1).length_squared() < eps || (p1 - p2).length_squared() < eps || (p2 - p0).length_squared() < eps) {
					continue;
				}

				indices.push_back(vertex_ofs + rindices[j * 3 + 0]);
				indices.push_back(vertex_ofs + rindices[j * 3 + 1]);
				indices.push_back(vertex_ofs + rindices[j * 3 + 2]);
			}
		}

		lightmap_surfaces.push_back(s);
	}

	//unwrap

	bool use_cache = true; // Used to request cache generation and to know if cache was used
	uint8_t *gen_cache;
	int gen_cache_size;
	float *gen_uvs;
	int *gen_vertices;
	int *gen_indices;
	int gen_vertex_count;
	int gen_index_count;
	int size_x;
	int size_y;

	bool ok = array_mesh_lightmap_unwrap_callback(p_texel_size, vertices.ptr(), normals.ptr(), vertices.size() / 3, indices.ptr(), indices.size(), p_src_cache.ptr(), &use_cache, &gen_cache, &gen_cache_size, &gen_uvs, &gen_vertices, &gen_vertex_count, &gen_indices, &gen_index_count, &size_x, &size_y);

	if (!ok) {
		return ERR_CANT_CREATE;
	}

	//remove surfaces
	clear();

	//create surfacetools for each surface..
	LocalVector<Ref<SurfaceTool>> surfaces_tools;

	for (int i = 0; i < lightmap_surfaces.size(); i++) {
		Ref<SurfaceTool> st;
		st.instantiate();
		st->begin(Mesh::PRIMITIVE_TRIANGLES);
		st->set_material(lightmap_surfaces[i].material);
		st->set_meta("name", lightmap_surfaces[i].name);
		surfaces_tools.push_back(st); //stay there
	}

	print_verbose("Mesh: Gen indices: " + itos(gen_index_count));

	//go through all indices
	for (int i = 0; i < gen_index_count; i += 3) {
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 0]], (int)uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 1]], (int)uv_indices.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 2]], (int)uv_indices.size(), ERR_BUG);

		ERR_FAIL_COND_V(uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 1]]].first || uv_indices[gen_vertices[gen_indices[i + 0]]].first != uv_indices[gen_vertices[gen_indices[i + 2]]].first, ERR_BUG);

		int surface = uv_indices[gen_vertices[gen_indices[i + 0]]].first;

		for (int j = 0; j < 3; j++) {
			SurfaceTool::Vertex v = lightmap_surfaces[surface].vertices[uv_indices[gen_vertices[gen_indices[i + j]]].second];

			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_COLOR) {
				surfaces_tools[surface]->set_color(v.color);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_TEX_UV) {
				surfaces_tools[surface]->set_uv(v.uv);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_NORMAL) {
				surfaces_tools[surface]->set_normal(v.normal);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_TANGENT) {
				Plane t;
				t.normal = v.tangent;
				t.d = v.binormal.dot(v.normal.cross(v.tangent)) < 0 ? -1 : 1;
				surfaces_tools[surface]->set_tangent(t);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_BONES) {
				surfaces_tools[surface]->set_bones(v.bones);
			}
			if (lightmap_surfaces[surface].format & Mesh::ARRAY_FORMAT_WEIGHTS) {
				surfaces_tools[surface]->set_weights(v.weights);
			}

			Vector2 uv2(gen_uvs[gen_indices[i + j] * 2 + 0], gen_uvs[gen_indices[i + j] * 2 + 1]);
			surfaces_tools[surface]->set_uv2(uv2);

			surfaces_tools[surface]->add_vertex(v.vertex);
		}
	}

	//generate surfaces
	for (unsigned int i = 0; i < surfaces_tools.size(); i++) {
		surfaces_tools[i]->index();
		Array arrays = surfaces_tools[i]->commit_to_arrays();
		add_surface(surfaces_tools[i]->get_primitive(), arrays, Array(), Dictionary(), surfaces_tools[i]->get_material(), surfaces_tools[i]->get_meta("name"));
	}

	set_lightmap_size_hint(Size2(size_x, size_y));

	if (gen_cache_size > 0) {
		r_dst_cache.resize(gen_cache_size);
		memcpy(r_dst_cache.ptrw(), gen_cache, gen_cache_size);
		memfree(gen_cache);
	}

	if (!use_cache) {
		// Cache was not used, free the buffers
		memfree(gen_vertices);
		memfree(gen_indices);
		memfree(gen_uvs);
	}

	return OK;
}

void EditorSceneImporterMesh::set_lightmap_size_hint(const Size2i &p_size) {
	lightmap_size_hint = p_size;
}

Size2i EditorSceneImporterMesh::get_lightmap_size_hint() const {
	return lightmap_size_hint;
}

void EditorSceneImporterMesh::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_blend_shape", "name"), &EditorSceneImporterMesh::add_blend_shape);
	ClassDB::bind_method(D_METHOD("get_blend_shape_count"), &EditorSceneImporterMesh::get_blend_shape_count);
	ClassDB::bind_method(D_METHOD("get_blend_shape_name", "blend_shape_idx"), &EditorSceneImporterMesh::get_blend_shape_name);

	ClassDB::bind_method(D_METHOD("set_blend_shape_mode", "mode"), &EditorSceneImporterMesh::set_blend_shape_mode);
	ClassDB::bind_method(D_METHOD("get_blend_shape_mode"), &EditorSceneImporterMesh::get_blend_shape_mode);

	ClassDB::bind_method(D_METHOD("add_surface", "primitive", "arrays", "blend_shapes", "lods", "material", "name"), &EditorSceneImporterMesh::add_surface, DEFVAL(Array()), DEFVAL(Dictionary()), DEFVAL(Ref<Material>()), DEFVAL(String()));

	ClassDB::bind_method(D_METHOD("get_surface_count"), &EditorSceneImporterMesh::get_surface_count);
	ClassDB::bind_method(D_METHOD("get_surface_primitive_type", "surface_idx"), &EditorSceneImporterMesh::get_surface_primitive_type);
	ClassDB::bind_method(D_METHOD("get_surface_name", "surface_idx"), &EditorSceneImporterMesh::get_surface_name);
	ClassDB::bind_method(D_METHOD("get_surface_arrays", "surface_idx"), &EditorSceneImporterMesh::get_surface_arrays);
	ClassDB::bind_method(D_METHOD("get_surface_blend_shape_arrays", "surface_idx", "blend_shape_idx"), &EditorSceneImporterMesh::get_surface_blend_shape_arrays);
	ClassDB::bind_method(D_METHOD("get_surface_lod_count", "surface_idx"), &EditorSceneImporterMesh::get_surface_lod_count);
	ClassDB::bind_method(D_METHOD("get_surface_lod_size", "surface_idx", "lod_idx"), &EditorSceneImporterMesh::get_surface_lod_size);
	ClassDB::bind_method(D_METHOD("get_surface_lod_indices", "surface_idx", "lod_idx"), &EditorSceneImporterMesh::get_surface_lod_indices);
	ClassDB::bind_method(D_METHOD("get_surface_material", "surface_idx"), &EditorSceneImporterMesh::get_surface_material);

	ClassDB::bind_method(D_METHOD("set_surface_name", "surface_idx", "name"), &EditorSceneImporterMesh::set_surface_name);
	ClassDB::bind_method(D_METHOD("set_surface_material", "surface_idx", "material"), &EditorSceneImporterMesh::set_surface_material);

	ClassDB::bind_method(D_METHOD("get_mesh", "base_mesh"), &EditorSceneImporterMesh::get_mesh, DEFVAL(Ref<ArrayMesh>()));
	ClassDB::bind_method(D_METHOD("clear"), &EditorSceneImporterMesh::clear);

	ClassDB::bind_method(D_METHOD("_set_data", "data"), &EditorSceneImporterMesh::_set_data);
	ClassDB::bind_method(D_METHOD("_get_data"), &EditorSceneImporterMesh::_get_data);

	ClassDB::bind_method(D_METHOD("set_lightmap_size_hint", "size"), &EditorSceneImporterMesh::set_lightmap_size_hint);
	ClassDB::bind_method(D_METHOD("get_lightmap_size_hint"), &EditorSceneImporterMesh::get_lightmap_size_hint);

	ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR), "_set_data", "_get_data");
}
