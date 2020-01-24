 All content © 2018 DigiPen (USA) Corporation, all rights reserved.
 Primary author Sangdae Kim

#ifndef HEADER_LIB_GAM_CONVEX
#define HEADER_LIB_GAM_CONVEX

#include vector
#include list

#include glmglm.hpp
#include glmgtcmatrix_transform.hpp
#include glmgtctype_ptr.hpp

#include triangle.hpp

namespace lib_gam {
	class convex {
	public
		struct label_vert {
			enum label {
				in,
				out,
				entering_inter,
				leaving_inter
			};

			glmvec2 vertex;
			label labeled;

			label_vert(glmvec2 vertex_, label labeled_)
			 vertex(vertex_), labeled(labeled_) {
			}
		};
	private

		static unsigned cyclic_idx(const unsigned current_idx_,
						const int step_,
						const stdsize_t size_);

		static bool are_verts_same(const glmvec2 a_,
						const glmvec2 b_);

		static void determine_vert(const unsigned prev_adj_idx_,
						const unsigned curr_idx_,
						const unsigned next_adj_idx_,
						const stdvectorglmvec2 &point_set_,
						bool &is_convex_);

		static bool is_ear(const triangle &tri_,
					const stdvectorglmvec2 &point_set_,
					const stdlistint reflexes_ = stdlistint());
		static unsigned walk_vertice(const unsigned start_idx_,
						const int amount_,
						const unsigned num_of_vertices_);
		static stdvectorconvexlabel_vert sort_ccw(const stdvectorconvexlabel_vert &point_set_);

	public
		enum orientation {
			None,
			CCW,
			CW,
			CL
		};

		static orientation calc_orientation(const glmvec2 prev_,
							const glmvec2 current_,
							const glmvec2 next_);
		static bool check_points_are_convex(const stdvectorglmvec2 points_);
		static stdvectorglmvec2 generate_convex_points(const stdvectorglmvec2 point_set_);
		static stdvectortriangle triangulate(const stdvectorglmvec2
									&point_set_);
		static stdvectorstdvectorglmvec2 triangulated(stdvectorglmvec2 point_set_);
		static bool check_point_in_triangle(const glmvec2 to_check_,
							const glmvec2 a_,
							const glmvec2 b_,
							const glmvec2 c_,
							const bool check_contain = false);
		static float cross_product(const glmvec2 a_, const glmvec2 b_);
		static glmvec2 perpendicular(const glmvec2 vec_);
		static float magnitude_squared(const glmvec2 vec_);
		static float magnitude(const glmvec2 vec_);

		static glmvec2 calc_centroid(stdvectorglmvec2 verts_,
						stdvectortriangle tris_);

		static bool clip(const stdvectorglmvec2 &clip_poly_,
					const stdvectortriangle &clip_tris_,
					const stdvectorglmvec2 &subject_poly_,
					const stdvectortriangle &subject_tris_,
					stdvectorglmvec2 &clipped_poly_,
					stdvectortriangle &clipped_tris_,
					stdvectorstdvectorconvexlabel_vert &excluded_polys_);
	};
}
#endif  HEADER_LIB_GAM_CONVEX