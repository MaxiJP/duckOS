/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright © 2016-2023 Byteduck */

#pragma once

#include <libduck/Object.h>
#include <libgraphics/Framebuffer.h>
#include "BufferSet.h"
#include "MatrixUtil.h"
#include "Vertex.h"
#include "Texture.h"
#include <array>
#include <utility>

namespace Lib3D {
	class RenderContext: public Duck::Object {
	public:
		DUCK_OBJECT_DEF(RenderContext);

		/// Getters and setters
		BufferSet& buffers() { return m_buffers; }
		const Gfx::Rect viewport() const { return m_viewport; }
		void set_viewport(Gfx::Rect rect);

		/// Projection and Matrices
		Vec4f constexpr project(Vec4f point) {
			return (m_premultmat * point).col(0);
		}

		template<typename T, size_t N>
		Vec3f constexpr screenspace(Vec<T, N> point) {
			return {(point.x() + 1.0f) * 0.5f * m_viewport.width,
					(-point.y() + 1.0f) * 0.5f * m_viewport.height,
					point.z()};
		}

		void set_modelmat(Matrix4f modelmat) {
			m_modelmat = modelmat;
			m_premultmat = m_projmat * m_modelmat;
		}

		void set_projmat(Matrix4f projmat) {
			m_projmat = projmat;
			m_premultmat = m_projmat * m_modelmat;
		}

		void set_depth_testing(bool depth_testing) { m_depth_testing = depth_testing; }
		void set_backface_culling(bool backface_culling) { m_backface_culling = backface_culling; }
		void set_alpha_testing(bool alpha_testing) { m_alpha_testing = alpha_testing; }

		/// Various Stuff
		void clear(Vec4f color);

		/// Drawing
		void line(Vertex a, Vertex b);
		void tri(std::array<Vertex, 3> verts);

		/// Textures
		void bind_texture(Texture* texture) { m_bound_texture = texture; }

	private:
		explicit RenderContext(Gfx::Dimensions dimensions);

		void tri_barycentric(std::array<Vertex, 3> verts);
		void tri_wireframe(std::array<Vertex, 3> verts);

		Matrix4f m_modelmat = identity<float, 4>();
		Matrix4f m_projmat  = ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		Matrix4f m_premultmat = m_projmat * m_modelmat;
		Gfx::Rect m_viewport;
		BufferSet m_buffers;
		Texture* m_bound_texture = nullptr;
		bool m_depth_testing = true;
		bool m_backface_culling = true;
		bool m_alpha_testing = false;
	};
}
