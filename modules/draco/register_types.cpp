/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "register_types.h"
#include "core/error/error_macros.h"
#include "core/variant/array.h"
#include "core/variant/dictionary.h"
#include "draco/compression/decode.h"
#include "draco/core/status.h"
#include "scene/resources/mesh.h"
#include "scene/resources/mesh_data_tool.h"
#include "scene/resources/surface_tool.h"

#include "draco/attributes/geometry_indices.h"
#include "draco/attributes/point_attribute.h"
#include "draco/compression/attributes/mesh_attribute_indices_encoding_data.h"
#include "draco/compression/encode.h"
#include "draco/compression/mesh/mesh_edgebreaker_decoder.h"
#include "draco/core/draco_types.h"
#include "draco/mesh/triangle_soup_mesh_builder.h"
#include "servers/rendering_server.h"
#include <stdint.h>
#include <memory>

float draco_attribute_quantization_func(ImporterMesh *p_importer_mesh, ImporterMesh *r_importer_mesh, int p_position_bits = 14, int p_normal_bits = 10, int p_uv_bits = 12, int p_other_attributes_bits = 32) {
	ERR_FAIL_NULL_V(p_importer_mesh, 0.0f);
	ERR_FAIL_NULL_V(r_importer_mesh, 0.0f);
	// Assumed deindexed
	draco::TriangleSoupMeshBuilder mesh_builder;
	Ref<MeshDataTool> mdt;
	mdt.instantiate();
	int32_t surface_count = p_importer_mesh->get_surface_count();

	const Ref<ArrayMesh> mesh = p_importer_mesh->get_mesh();
	int64_t points = 0;
	int64_t quantized_points = 0;
	for (int32_t surface_i = 0; surface_i < surface_count; surface_i++) {
		mdt->create_from_surface(mesh, surface_i);
		mesh_builder.Start(mdt->get_face_count());
		const int32_t pos_att_id = mesh_builder.AddAttribute(
				draco::GeometryAttribute::POSITION, 3, draco::DT_FLOAT32);
		const int32_t normal_att_id = mesh_builder.AddAttribute(
				draco::GeometryAttribute::NORMAL, 3, draco::DT_FLOAT32);
		const int32_t tex_att_id_0 = mesh_builder.AddAttribute(
				draco::GeometryAttribute::TEX_COORD, 2, draco::DT_FLOAT32);
		const int32_t tex_att_id_1 = mesh_builder.AddAttribute(
				draco::GeometryAttribute::TEX_COORD, 2, draco::DT_FLOAT32);

		// Todo: fire 2022-01-25T00:50:51-0800 - 1-8 uvs, blend shapes, tangents, and bitangents
		int32_t bone_weight_count = 0;
		if (mdt->get_format() & Mesh::ARRAY_FORMAT_BONES && (mdt->get_format() & Mesh::ARRAY_FLAG_USE_8_BONE_WEIGHTS)) {
			bone_weight_count = 8;
		} else if (mdt->get_format() & Mesh::ARRAY_FORMAT_BONES) {
			bone_weight_count = 4;
		}
		for (int32_t face_i = 0; face_i < mdt->get_face_count(); face_i++) {
			int32_t vert_0 = mdt->get_face_vertex(face_i, 0);
			int32_t vert_1 = mdt->get_face_vertex(face_i, 1);
			int32_t vert_2 = mdt->get_face_vertex(face_i, 2);
			Vector3 pos_0 = mdt->get_vertex(vert_0);
			Vector3 pos_1 = mdt->get_vertex(vert_1);
			Vector3 pos_2 = mdt->get_vertex(vert_2);
			mesh_builder.SetAttributeValuesForFace(
					pos_att_id, draco::FaceIndex(face_i), &pos_0, &pos_1, &pos_2);
			Vector3 norm_0 = mdt->get_vertex_normal(vert_0);
			Vector3 norm_1 = mdt->get_vertex_normal(vert_1);
			Vector3 norm_2 = mdt->get_vertex_normal(vert_2);
			mesh_builder.SetAttributeValuesForFace(
					normal_att_id, draco::FaceIndex(face_i), &norm_0, &norm_1, &norm_2);
			Vector2 uv_0 = mdt->get_vertex_uv(vert_0);
			Vector2 uv_1 = mdt->get_vertex_uv(vert_1);
			Vector2 uv_2 = mdt->get_vertex_uv(vert_2);
			mesh_builder.SetAttributeValuesForFace(tex_att_id_0, draco::FaceIndex(face_i), &uv_0, &uv_1, &uv_2);
			Vector2 uv2_0 = mdt->get_vertex_uv2(vert_0);
			Vector2 uv2_1 = mdt->get_vertex_uv2(vert_1);
			Vector2 uv2_2 = mdt->get_vertex_uv2(vert_2);
			mesh_builder.SetAttributeValuesForFace(tex_att_id_1, draco::FaceIndex(face_i), &uv2_0, &uv2_1, &uv2_2);
		}
		int32_t bone_attr_id = -1;
		int32_t bone_weight_attr_id = -1;
		if (bone_weight_count) {
			bone_attr_id = mesh_builder.AddAttribute(
					draco::GeometryAttribute::GENERIC, bone_weight_count, draco::DT_INT32);
			bone_weight_attr_id = mesh_builder.AddAttribute(
					draco::GeometryAttribute::GENERIC, bone_weight_count, draco::DT_FLOAT32);
			for (int32_t face_i = 0; face_i < mdt->get_face_count(); face_i++) {
				int32_t vert_0 = mdt->get_face_vertex(face_i, 0);
				int32_t vert_1 = mdt->get_face_vertex(face_i, 1);
				int32_t vert_2 = mdt->get_face_vertex(face_i, 2);
				Vector<int> bones_0 = mdt->get_vertex_bones(vert_0);
				Vector<int> bones_1 = mdt->get_vertex_bones(vert_1);
				Vector<int> bones_2 = mdt->get_vertex_bones(vert_2);
				mesh_builder.SetAttributeValuesForFace(
						bone_attr_id, draco::FaceIndex(face_i), bones_0.ptrw(), bones_1.ptrw(), bones_2.ptrw());
				Vector<float> weights_0 = mdt->get_vertex_weights(vert_0);
				Vector<float> weights_1 = mdt->get_vertex_weights(vert_1);
				Vector<float> weights_2 = mdt->get_vertex_weights(vert_2);
				mesh_builder.SetAttributeValuesForFace(
						bone_weight_attr_id, draco::FaceIndex(face_i), weights_0.ptrw(), weights_1.ptrw(), weights_2.ptrw());
			}
		}
		std::unique_ptr<draco::Mesh> draco_mesh = mesh_builder.Finalize();
		points += draco_mesh->num_faces() * 3;
		draco::Encoder encoder;
		encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 12);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, 10);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, 30);
		encoder.SetSpeedOptions(0, 5);
		draco::EncoderBuffer buffer;
		encoder.EncodeMeshToBuffer(*draco_mesh, &buffer);
		draco::DecoderBuffer in_buffer;
		in_buffer.Init(buffer.data(), buffer.size());
		draco::Decoder decoder;
		draco::DecoderOptions options;
		draco::StatusOr<std::unique_ptr<draco::Mesh>> status_or_mesh = decoder.DecodeMeshFromBuffer(&in_buffer);
		draco::Status status = status_or_mesh.status();
		ERR_CONTINUE_MSG(status.code() != draco::Status::OK, vformat("Error decoding draco buffer '%s' message %d\n", status.error_msg(), status.code()));
		const std::unique_ptr<draco::Mesh> &output_draco_mesh = status_or_mesh.value();
		ERR_FAIL_COND_V(!output_draco_mesh, 0.0f);
		quantized_points += output_draco_mesh->num_faces() * 3;
		Ref<SurfaceTool> surface_tool;
		surface_tool.instantiate();
		if (bone_weight_count == 8) {
			surface_tool->set_skin_weight_count(SurfaceTool::SKIN_8_WEIGHTS);
		}
		surface_tool->begin(Mesh::PrimitiveType::PRIMITIVE_TRIANGLES);
		for (draco::FaceIndex face_i(0); face_i < output_draco_mesh->num_faces(); ++face_i) {
			for (int32_t tri_i = 0; tri_i < 3; tri_i++) {
				if (bone_weight_count) {
					int bone_val[8];
					float weight_val[8];
					const draco::PointAttribute *bone_attr = output_draco_mesh->GetAttributeByUniqueId(bone_attr_id);
					bone_attr->GetMappedValue(output_draco_mesh->face(face_i)[tri_i], bone_val);
					Vector<int> bones;
					bones.resize(bone_weight_count);
					for (int32_t bone_i = 0; bone_i < bone_weight_count; bone_i++) {
						bones.write[bone_i] = bone_val[bone_i];
					}
					surface_tool->set_bones(bones);
					const draco::PointAttribute *weight_attr = output_draco_mesh->GetAttributeByUniqueId(bone_weight_attr_id);
					weight_attr->GetMappedValue(output_draco_mesh->face(face_i)[tri_i], weight_val);
					Vector<float> weights;
					weights.resize(bone_weight_count);
					for (int32_t bone_i = 0; bone_i < bone_weight_count; bone_i++) {
						weights.write[bone_i] = weight_val[bone_i];
					}
					surface_tool->set_weights(weights);
				}
				float pos_val[3];
				float normal_val[3];
				float uv0_val[2];
				const draco::PointAttribute *normal_attr = output_draco_mesh->GetAttributeByUniqueId(normal_att_id);
				normal_attr->GetMappedValue(output_draco_mesh->face(face_i)[tri_i], normal_val);
				surface_tool->set_normal(Vector3(normal_val[0], normal_val[1], normal_val[2]));
				const draco::PointAttribute *tex_0_attr = output_draco_mesh->GetAttributeByUniqueId(tex_att_id_0);
				tex_0_attr->GetMappedValue(output_draco_mesh->face(face_i)[tri_i], uv0_val);
				surface_tool->set_uv(Vector2(uv0_val[0], uv0_val[1]));
				const draco::PointAttribute *point_attr = output_draco_mesh->GetAttributeByUniqueId(pos_att_id);
				ERR_BREAK(!point_attr);
				point_attr->GetMappedValue(output_draco_mesh->face(face_i)[tri_i], pos_val);
				surface_tool->add_vertex(Vector3(pos_val[0], pos_val[1], pos_val[2]));
			}
		}
		surface_tool->index();
		r_importer_mesh->add_surface(Mesh::PRIMITIVE_TRIANGLES,
				surface_tool->commit_to_arrays(), Array(), Dictionary(), mesh->surface_get_material(surface_i), mesh->surface_get_name(surface_i));
		// TODO: fire 2022-01-24T18:08:35-0800 Handle all surfaces including blend shapes
	}
	return (float)points / quantized_points;
}

void register_draco_types() {
	SurfaceTool::attribute_quantization_func = draco_attribute_quantization_func;
}

void unregister_draco_types() {
	SurfaceTool::attribute_quantization_func = nullptr;
}
