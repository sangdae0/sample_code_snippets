// All content © 2018 DigiPen (USA) Corporation, all rights reserved.
// Primary author: Sangdae Kim

#include "convex.hpp"
#include <list>
#include <algorithm>
#include <iostream>

using namespace lib_gam;

// https://www.geeksforgeeks.org/orientation-3-ordered-points/
convex::orientation convex::calc_orientation(const glm::vec2 prev_,
						const glm::vec2 current_,
						const glm::vec2 next_) {
		// https://stackoverflow.com/a/49764094
		const auto cp = (current_.y - prev_.y) * (next_.x - current_.x)
				- (current_.x - prev_.x) * (next_.y - current_.y);
		if (cp > 0)
			return convex::orientation::CCW;
		else if (cp < 0)
			return convex::orientation::CW;
		else
			return convex::orientation::CL;
}

bool convex::check_points_are_convex(std::vector<glm::vec2> points_) {
	// check if points are ccw
	// https://www.urbanpro.com/gre/how-to-determine-if-points-are-collinear
	const auto pn = points_.size();
	for (unsigned i = 0; i < pn; ++ i) {
		const auto prev = points_[i == 0 ? pn - 1 : i - 1];
		const auto current = points_[i];
		const auto next = points_[(i + 1) >= pn ? 0 : i + 1];

		const auto orientation = calc_orientation(prev, current, next);

		if (orientation != convex::orientation::CCW)
			return false;
	}
	return true;
}

std::vector<glm::vec2> convex::generate_convex_points(const std::vector<glm::vec2> point_set_) {
	// check convex
	// https://www.geeksforgeeks.org/convex-hull-set-1-jarviss-algorithm-or-wrapping/
	// https://en.wikipedia.org/wiki/Gift_wrapping_algorithm
	// find leftmost
	const auto pn = point_set_.size();

	auto lowest = std::numeric_limits<float>::max();
	auto lowest_idx = pn;
	for (unsigned i = 0; i < pn; ++ i) {
		if (const auto p_x = point_set_[i].x;
			p_x < lowest) {
			lowest = p_x;
			lowest_idx = i;
		}
	}

	assert(lowest_idx != pn);

	auto last_hull_point_idx = lowest_idx;
	std::vector<glm::vec2> convexes;
	convexes.push_back(point_set_[last_hull_point_idx]);
	auto end_condition = false;
	unsigned check_count = 0;
	do {
		// https://math.stackexchange.com/a/361419
		const auto cn = convexes.size();
		const auto current_convex = convexes[cn - 1];
		const auto un_last_convex_edge = (cn < 2) ? glm::vec2(1, 0) : glm::normalize(convexes[cn - 2] - current_convex);
		auto max_angle_found = std::numeric_limits<float>::lowest();
		auto idx_next_convex_candidate = pn;
		auto max_collinear_dist_squared_found = std::numeric_limits<float>::lowest();
		for (unsigned j = 0; j < pn; ++ j) {
			if (j == last_hull_point_idx)
				continue;
			const auto pt_to_compare = point_set_[j];
			const auto to_point = pt_to_compare - current_convex;
			const auto un_to_point = glm::normalize(to_point);
			if (const auto angle_between = glm::acos(glm::dot(un_last_convex_edge, un_to_point));
				angle_between >= max_angle_found) {
				max_angle_found = angle_between;
				// need 3 points to determine orientation
				if (!(cn < 2))
					idx_next_convex_candidate = j;
				// exclude collinear
				else if (const auto orientation = convex::calc_orientation(convexes[cn - 2], current_convex, pt_to_compare);
					orientation == convex::orientation::CCW) {
					idx_next_convex_candidate = j;
				} else if (orientation == convex::orientation::CL) {
					// https://stackoverflow.com/a/32317564
					if (const auto dist_squared = to_point.x * to_point.x + to_point.y * to_point.y;
							dist_squared > max_collinear_dist_squared_found) {
						max_collinear_dist_squared_found = dist_squared;
						idx_next_convex_candidate = j;
					}
				}
			}
		}
		assert (idx_next_convex_candidate != pn);
		// prevent infinite loop
		++ check_count;
		end_condition = (idx_next_convex_candidate == lowest_idx) || (check_count >= pn);
		if (!end_condition) {
			last_hull_point_idx = idx_next_convex_candidate;
			convexes.push_back(point_set_[last_hull_point_idx]);
		}
	} while (!end_condition);

	return convexes;
}

std::vector<triangle> convex::triangulate(const std::vector<glm::vec2> &point_set_) {
	// implementation of https://www.geometrictools.com/Documentation/TriangulationByEarClipping.pdf
	const auto pn = static_cast<int>(point_set_.size());
	assert(pn > 2);

	std::vector<triangle> tris;

	// don't need triangluation
	if (pn == 3) {
		tris.push_back(triangle(0, 1, 2));
		return tris;
	}

	std::list<int> vertices; // cyclic
	for (unsigned i = 0; i < pn; ++ i)
		vertices.push_back(static_cast<int>(i));

	std::list<int> reflex_idxs; // linear
	std::list<int> convex_idxs; // linear
	std::list<int> ear_tips; // cyclick

	// detect ears of polygon
	for (int i = 0; i < pn; ++ i) {
		const auto curr_vert = point_set_[static_cast<unsigned>(i)];
		const auto i_prev = (i - 1) < 0 ? pn - 1 : i - 1;
		const auto prev_vert = point_set_[static_cast<unsigned>(i_prev)];
		const auto i_next = (i + 1) >= pn ? 0 : i + 1;
		const auto next_vert = point_set_[static_cast<unsigned>(i_next)];

		// check convex or reflex
		const auto vec_a = prev_vert - curr_vert;
		auto angle_a = std::atan2(vec_a.y, vec_a.x);
		const auto vec_b = next_vert - curr_vert;
		auto angle_b = std::atan2(vec_b.y, vec_b.x);
		auto angle_between = angle_b - angle_a;
		// https://stackoverflow.com/a/21484228
		const auto pi_2 = 2 * glm::pi<float>();
		if (angle_between < 0)
			angle_between += pi_2;

		if (angle_between > glm::pi<float>())
			reflex_idxs.push_back(static_cast<int>(i));
		else
			convex_idxs.push_back(static_cast<int>(i));
	}


	for (int i = 0; i < pn; ++ i) {
		const auto curr_vert = point_set_[static_cast<unsigned>(i)];
		const auto i_prev = (i - 1) < 0 ? pn - 1 : i - 1;
		const auto prev_vert = point_set_[static_cast<unsigned>(i_prev)];
		const auto i_next = (i + 1) >= pn ? 0 : i + 1;
		const auto next_vert = point_set_[static_cast<unsigned>(i_next)];

		const auto tri = triangle(static_cast<unsigned>(i_prev),
						static_cast<unsigned>(i),
						static_cast<unsigned>(i_next));
		if (is_ear(tri, point_set_, reflex_idxs))
			ear_tips.push_back(static_cast<int>(i));
	}

	while (vertices.size() >= 3 && ear_tips.size() > 0) {
		int ear_tip = ear_tips.front();
		ear_tips.pop_front();

		auto found = std::find(vertices.begin(), vertices.end(), ear_tip);
		auto prev_adj_it = found;
		if (prev_adj_it == vertices.begin()) {
			prev_adj_it = vertices.end();
			-- prev_adj_it;
		} else {
			-- prev_adj_it;
		}

		auto next_adj_it = found;
		++ next_adj_it;
		if (next_adj_it == vertices.end())
			next_adj_it = vertices.begin();

		vertices.erase(found);
		const auto is_in_reflex = std::find(reflex_idxs.begin(), reflex_idxs.end(), ear_tip) != reflex_idxs.end();
		if (is_in_reflex)
			reflex_idxs.remove(ear_tip);
		const auto is_in_convex = std::find(convex_idxs.begin(), convex_idxs.end(), ear_tip) != convex_idxs.end();
		if (is_in_convex)
			convex_idxs.remove(ear_tip);

		const auto tri = triangle(static_cast<unsigned>(*prev_adj_it),
				static_cast<unsigned>(ear_tip),
				static_cast<unsigned>(*next_adj_it));
		tris.push_back(tri);

		auto prev_prev_adj_it = prev_adj_it;
		if (prev_prev_adj_it == vertices.begin()) {
			prev_prev_adj_it = vertices.end();
			-- prev_prev_adj_it;
		} else {
			-- prev_prev_adj_it;
		}

		{
			bool is_convex;
			determine_vert(static_cast<unsigned>(*prev_prev_adj_it),
					static_cast<unsigned>(*prev_adj_it),
					static_cast<unsigned>(*next_adj_it),
					point_set_,
					is_convex);
			const auto idx = *prev_adj_it;
			if (!is_convex) {
				const auto is_in_reflex = std::find(reflex_idxs.begin(), reflex_idxs.end(), idx) != reflex_idxs.end();
				if (!is_in_reflex)
					reflex_idxs.push_back(idx);
				const auto is_in_convex = std::find(convex_idxs.begin(), convex_idxs.end(), idx) != convex_idxs.end();
				if (is_in_convex)
					convex_idxs.remove(idx);
				const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), idx) != ear_tips.end();
				if (is_in_ear)
					ear_tips.remove(idx);
			} else {
				const auto is_in_reflex = std::find(reflex_idxs.begin(), reflex_idxs.end(), idx) != reflex_idxs.end();
				if (is_in_reflex)
					reflex_idxs.remove(idx);
				const auto is_in_convex = std::find(convex_idxs.begin(), convex_idxs.end(), idx) != convex_idxs.end();
				if (!is_in_convex)
					convex_idxs.push_back(idx);
			}
		}

		auto next_next_adj_it = next_adj_it;
		++ next_next_adj_it;
		if (next_next_adj_it == vertices.end())
			next_next_adj_it = vertices.begin();

		{

			bool is_convex;
			determine_vert(static_cast<unsigned>(*prev_adj_it),
					static_cast<unsigned>(*next_adj_it),
					static_cast<unsigned>(*next_next_adj_it),
					point_set_,
					is_convex);
			const auto idx = *next_adj_it;
			if (!is_convex) {
				const auto is_in_reflex = std::find(reflex_idxs.begin(), reflex_idxs.end(), idx) != reflex_idxs.end();
				if (!is_in_reflex)
					reflex_idxs.push_back(idx);
				const auto is_in_convex = std::find(convex_idxs.begin(), convex_idxs.end(), idx) != convex_idxs.end();
				if (is_in_convex)
					convex_idxs.remove(idx);
				const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), idx) != ear_tips.end();
				if (is_in_ear)
					ear_tips.remove(idx);
			} else {
				const auto is_in_reflex = std::find(reflex_idxs.begin(), reflex_idxs.end(), idx) != reflex_idxs.end();
				if (is_in_reflex)
					reflex_idxs.remove(idx);
				const auto is_in_convex = std::find(convex_idxs.begin(), convex_idxs.end(), idx) != convex_idxs.end();
				if (!is_in_convex)
					convex_idxs.push_back(idx);
			}
		}

		if (const auto tri = triangle(static_cast<unsigned>(*prev_prev_adj_it),
						static_cast<unsigned>(*prev_adj_it),
						static_cast<unsigned>(*next_adj_it));
			is_ear(tri,
				point_set_,
				reflex_idxs)) {
			const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), *prev_adj_it) != ear_tips.end();
			if (!is_in_ear)
				ear_tips.push_back(*prev_adj_it);
		} else {
			const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), *prev_adj_it) != ear_tips.end();
			if (is_in_ear)
				ear_tips.remove(*prev_adj_it);
		}

		if (const auto tri = triangle(static_cast<unsigned>(*prev_adj_it),
						static_cast<unsigned>(*next_adj_it),
						static_cast<unsigned>(*next_next_adj_it));
			is_ear(tri,
				point_set_,
				reflex_idxs)) {
			const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), *next_adj_it) != ear_tips.end();
			if (!is_in_ear)
				ear_tips.push_back(*next_adj_it);
		} else {
			const auto is_in_ear = std::find(ear_tips.begin(), ear_tips.end(), *next_adj_it) != ear_tips.end();
			if (is_in_ear)
				ear_tips.remove(*next_adj_it);
		}
	}

	return tris;
}

unsigned convex::walk_vertice(const unsigned start_idx_,
				const int amount_,
				const unsigned num_of_vertices_) {
	assert(start_idx_ < num_of_vertices_);
	auto pos = static_cast<int>(start_idx_);
	auto count = 0;
	auto positive = amount_ > 0 ? true : false;
	const auto abs_amount = std::abs(amount_);
	while (count < abs_amount) {
		if (!positive)
			pos = pos - 1 < 0 ? static_cast<int>(num_of_vertices_) - 1 : pos - 1;
		else
			pos = pos + 1 >= static_cast<int>(num_of_vertices_) ? 0 : pos + 1;
		++ count;
	}
	return static_cast<unsigned>(pos);
}

std::vector<std::vector<glm::vec2>> convex::triangulated(std::vector<glm::vec2> point_set_) {
	const auto tris = triangulate(point_set_);
	std::vector<std::vector<glm::vec2>> ret;
	for (unsigned i = 0; i < tris.size(); ++ i ) {
		std::vector<glm::vec2> poly;
		poly.push_back(point_set_[tris[i].a]);
		poly.push_back(point_set_[tris[i].b]);
		poly.push_back(point_set_[tris[i].c]);
		ret.push_back(poly);
	}
	return ret;
}

// http://www.sunshine2k.de/coding/java/Polygon/Kong/Kong.html
bool convex::is_ear(const triangle &tri_,
				const std::vector<glm::vec2> &point_set_,
				const std::list<int> reflexes_) {
	const auto prev_adj_vert = tri_.a;
	const auto ear_tip = tri_.b;
	const auto next_adj_vert = tri_.c;
	const auto adj_pt = point_set_[prev_adj_vert];
	const auto ear_tip_pt = point_set_[ear_tip];
	const auto adj_pt2 = point_set_ [next_adj_vert];

	// check convex or reflex
	const auto vec_a = adj_pt - ear_tip_pt;
	auto angle_a = std::atan2(vec_a.y, vec_a.x);
	const auto vec_b = adj_pt2 - ear_tip_pt;
	auto angle_b = std::atan2(vec_b.y, vec_b.x);
	auto angle_between = angle_b - angle_a;
	// https://stackoverflow.com/a/21484228
	const auto pi_2 = 2 * glm::pi<float>();
	if (angle_between < 0)
		angle_between += pi_2;

	// not convex
	if (angle_between > glm::pi<float>())
		return false;

	const auto orientation = calc_orientation(adj_pt, ear_tip_pt, adj_pt2);
	if (orientation != orientation::CCW)
		return false;

	// check contains vertex
	// http://www.drdobbs.com/database/triangle-intersection-tests/184404201
	for (auto it = reflexes_.begin(); it != reflexes_.end(); ++ it) {
		const auto reflex = static_cast<unsigned>(*it);
		if (reflex == tri_.a || reflex == tri_.b || reflex == tri_.c)
			continue;
		const auto reflex_pt = point_set_[reflex];
		// get a triangle
		if (check_point_in_triangle(reflex_pt,
						adj_pt,
						ear_tip_pt,
						adj_pt2,
						true))
			return false;
	}

	/*
	for (unsigned long j = 0; j < point_set_.size(); ++ j) {
		if (j == tri_.a || j == tri_.b || j == tri_.c)
			continue;
		const auto point = point_set_[j];
		// get a triangle
		if (check_point_in_triangle(point,
						adj_pt,
						ear_tip_pt,
						adj_pt2,
						true))
			return false;
	}
	*/

	return true;
}

bool convex::check_point_in_triangle(const glm::vec2 to_check_,
					const glm::vec2 a_,
					const glm::vec2 b_,
					const glm::vec2 c_,
					const bool check_contain) {
	// https://www.youtube.com/watch?v=HYAgJN3x4GA/
	const auto w1 = (
				(a_.x * (c_.y - a_.y))
				+ ((to_check_.y - a_.y) * (c_.x - a_.x))
				- (to_check_.x * (c_.y - a_.y))
			)
			/
			(
				((b_.y - a_.y) * (c_.x - a_.x))
					- ((b_.x - a_.x) * (c_.y - a_.y))
			);

	if (check_contain) {
		if (w1 < 0)
			return false;
	} else {
		if (w1 <= 0)
			return false;
	}

	const auto w2 = (
				to_check_.y - a_.y - w1 * (b_.y - a_.y)
			)
			/
			(
				c_.y - a_.y
			);

	if (check_contain) {
		if (w2 < 0)
			return false;
	} else {
		if (w2 <= 0)
			return false;
	}

	if (check_contain) {
		if ((w1 + w2) > 1)
			return false;
	} else {
		if ((w1 + w2) >= 1)
			return false;
	}

	return true;
}

float convex::cross_product(const glm::vec2 a_, const glm::vec2 b_) {
	return a_.x * b_.y - a_.y * b_.x;
}

glm::vec2 convex::perpendicular(const glm::vec2 vec_) {
	return glm::vec2(-vec_.y, vec_.x);
}

float convex::magnitude_squared(const glm::vec2 vec_) {
	return vec_.x * vec_.x + vec_.y * vec_.y;
}

float convex::magnitude(const glm::vec2 vec_) {
	return glm::sqrt(vec_.x * vec_.x + vec_.y * vec_.y);
}

bool convex::clip(const std::vector<glm::vec2> &clip_poly_,
					const std::vector<triangle> &clip_tris_,
					const std::vector<glm::vec2> &subject_poly_,
					const std::vector<triangle> &subject_tris_,
					std::vector<glm::vec2> &clipped_poly_,
					std::vector<triangle> &clipped_tris_,
					std::vector<std::vector<convex::label_vert>> &excluded_polys_) {
	clipped_poly_.clear();
	clipped_tris_.clear();
	excluded_polys_.clear();

	std::vector<label_vert> clips;
	auto is_completely_inside = true;
	auto any_intersection_found = false;
	// http://www0.cs.ucl.ac.uk/staff/a.steed/book_tmp/CGVE/slides/clipping.ppt
	for (unsigned i = 0; i < clip_poly_.size(); ++ i) {
		const auto clip_a = clip_poly_[i % clip_poly_.size()];
		const auto clip_b = clip_poly_[(i + 1) % clip_poly_.size()];
		const auto clip_ab = clip_b - clip_a;

		std::vector<label_vert> inter_founds;
		for (unsigned j = 0; j < subject_poly_.size(); ++ j) {
			const auto epsilon = glm::epsilon<float>();
			const auto subject_c = subject_poly_[j % subject_poly_.size()];
			const auto subject_d = subject_poly_[(j + 1) % subject_poly_.size()];
			const auto subject_cd = subject_d - subject_c;
			// find intersection
			/* https://www.quora.com/
				How-do-I-get-the-point-of-intersection-of
				-two-lines-using-a-cross-product-if-I-know
				-two-points-of-each-line
			*/
			/* https://stackoverflow.com/questions/563198/how-do
				-you-detect-where-two-line-segments-intersect
			*/

			auto t = -1.0f;
			if (const auto cp_cd_ab = cross_product(subject_cd, clip_ab);
				glm::abs<float>(cp_cd_ab) >= glm::epsilon<float>()) {
				const auto cp_ca_clip = cross_product((clip_a - subject_c), clip_ab);
				t = cp_ca_clip / cp_cd_ab;
			}

			auto s = -1.0f;
			if (const auto cp_ab_cd = cross_product(clip_ab, subject_cd);
				glm::abs<float>(cp_ab_cd) >= epsilon) {
				const auto cp_ac_subject = cross_product((subject_c - clip_a), subject_cd);
				s = cp_ac_subject / cp_ab_cd;
			}
			const auto edge_un = convex::perpendicular(glm::normalize(subject_cd));

			const auto is_a_inside = glm::dot(edge_un, subject_c - clip_a) > epsilon
							&& glm::dot(edge_un, subject_d - clip_a) > epsilon;
			const auto is_b_inside = glm::dot(edge_un, subject_c - clip_b) > epsilon
							&& glm::dot(edge_un, subject_d - clip_b) > epsilon;

			if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
				any_intersection_found = true;
				const auto intersection = subject_c + t * (subject_cd);
				if (is_a_inside && !is_b_inside)
					inter_founds.push_back(label_vert(intersection, label_vert::label::leaving_inter));
				else if (!is_a_inside && is_b_inside)
					inter_founds.push_back(label_vert(intersection, label_vert::label::entering_inter));
			}
		}

		auto is_inside_vert = false;
		for (unsigned j = 0; j < subject_tris_.size(); ++ j) {
			const auto subject_tri = subject_tris_[j];
			if (check_point_in_triangle(clip_a,
							subject_poly_[subject_tri.a],
							subject_poly_[subject_tri.b],
							subject_poly_[subject_tri.c])) {

				is_inside_vert = true;
				break;
			}
		}

		if (is_inside_vert)
			clips.push_back(label_vert(clip_a, label_vert::label::in));
		else {
			clips.push_back(label_vert(clip_a, label_vert::label::out));
			is_completely_inside = false;
		}

		if (inter_founds.size() > 1) {
			const auto dist_1 = magnitude_squared(clip_a - inter_founds[0].vertex);
			const auto dist_2 = magnitude_squared(clip_a - inter_founds[inter_founds.size() - 1].vertex);
			const int reverse = dist_1 > dist_2 ? true : false;
			if (!reverse) {
				for (int j = 0; j < static_cast<int>(inter_founds.size()); ++ j) {
					const auto intersection = inter_founds[static_cast<unsigned>(j)];
					clips.push_back(intersection);
				}
			} else {
				for (int j = static_cast<int>(inter_founds.size() - 1); j >= 0; -- j) {
					const auto intersection = inter_founds[static_cast<unsigned>(j)];
					clips.push_back(intersection);
				}
			}
		} else {
			for (int j = 0; j < static_cast<int>(inter_founds.size()); ++ j) {
				const auto intersection = inter_founds[static_cast<unsigned>(j)];
				clips.push_back(intersection);
			}
		}
	}

	if (!any_intersection_found)
		return false;
	//else if (is_completely_inside) // no support hole case
	//	return false;

	std::list<label_vert> clipped_poly_verts;
	std::vector<label_vert> subjects;
	// https://www.youtube.com/watch?v=c065KoXooSw
	// https://youtu.be/LCMyWFxeuro
	// https://en.wikipedia.org/wiki/Weiler%E2%80%93Atherton_clipping_algorithm
	for (unsigned i = 0; i < subject_poly_.size(); ++ i) {
		const auto subject_c = subject_poly_[i % subject_poly_.size()];
		const auto subject_d = subject_poly_[(i + 1) % subject_poly_.size()];
		const auto subject_cd = subject_d - subject_c;

		std::vector<label_vert> inter_founds;
		for (unsigned j = 0; j < clip_poly_.size(); ++ j) {
			const auto epsilon = glm::epsilon<float>();
			const auto clip_a = clip_poly_[j % clip_poly_.size()];
			const auto clip_b = clip_poly_[(j + 1) % clip_poly_.size()];
			const auto clip_ab = clip_b - clip_a;

			// find intersection
			auto t = -1.0f;
			if (const auto cp_cd_ab = cross_product(subject_cd, clip_ab);
				glm::abs<float>(cp_cd_ab) >= epsilon) {
				const auto cp_ca_clip = cross_product((clip_a - subject_c), clip_ab);
				t = cp_ca_clip / cp_cd_ab;
			}

			auto s = -1.0f;
			if (const auto cp_ab_cd = cross_product(clip_ab, subject_cd);
				glm::abs<float>(cp_ab_cd) >= epsilon) {
				const auto cp_ac_subject = cross_product((subject_c - clip_a), subject_cd);
				s = cp_ac_subject / cp_ab_cd;
			}

			const auto edge_un = convex::perpendicular(glm::normalize(clip_ab));
			const auto is_c_inside = glm::dot(edge_un, clip_a - subject_c) > epsilon
							&& glm::dot(edge_un, clip_b - subject_c) > epsilon;
			const auto is_d_inside = glm::dot(edge_un, clip_a - subject_d) > epsilon
							&& glm::dot(edge_un, clip_b - subject_d) > epsilon;

			if (s >= 0.0f && s <= 1.0f && t >= 0.0f && t <= 1.0f) {
				const auto intersection = subject_c + t * (subject_cd);
				if (is_c_inside && !is_d_inside)
					inter_founds.push_back(label_vert(intersection, label_vert::label::leaving_inter));
				else if (!is_c_inside && is_d_inside)
					inter_founds.push_back(label_vert(intersection, label_vert::label::entering_inter));
			}
		}

		auto is_inside_vert = false;
		for (unsigned j = 0; j < clip_tris_.size(); ++ j) {
			const auto clip_tri = clip_tris_[j];
			if (check_point_in_triangle(subject_c,
							clip_poly_[clip_tri.a],
							clip_poly_[clip_tri.b],
							clip_poly_[clip_tri.c])) {
				is_inside_vert = true;
				break;
			}
		}

		if (is_inside_vert) {
			const auto vert_to_add = label_vert(subject_c, label_vert::label::in);
			subjects.push_back(vert_to_add);
			clipped_poly_verts.push_back(vert_to_add);
		} else {
			const auto vert_to_add = label_vert(subject_c, label_vert::label::out);
			subjects.push_back(vert_to_add);
			clipped_poly_verts.push_back(vert_to_add);
		}

		if (inter_founds.size() > 1) {
			const auto dist_1 = magnitude_squared(subject_c - inter_founds[0].vertex);
			const auto dist_2 = magnitude_squared(subject_c - inter_founds[inter_founds.size() - 1].vertex);
			const int reverse = dist_1 > dist_2 ? true : false;
			if (!reverse) {
				for (int j = 0; j < static_cast<int>(inter_founds.size()); ++ j) {
					const auto intersection = inter_founds[static_cast<unsigned>(j)];
					subjects.push_back(intersection);
					clipped_poly_verts.push_back(intersection);
				}
			} else {
				for (int j = static_cast<int>(inter_founds.size() - 1); j >= 0; -- j) {
					const auto intersection = inter_founds[static_cast<unsigned>(j)];
					subjects.push_back(intersection);
					clipped_poly_verts.push_back(intersection);
				}
			}
		} else {
			for (int j = 0; j < static_cast<int>(inter_founds.size()); ++ j) {
				const auto intersection = inter_founds[static_cast<unsigned>(j)];
				subjects.push_back(intersection);
				clipped_poly_verts.push_back(intersection);
			}
		}
	}

	unsigned start_idx = 0;
	unsigned current_idx = 0;
	auto start_idx_found = false;
	auto step = 1;
	const auto clips_size = static_cast<unsigned>(clips.size());
	do {
		current_idx = cyclic_idx(current_idx, step, clips_size);
		const auto current_vert = clips[current_idx];

		if (current_vert.labeled == label_vert::label::entering_inter) {
			const auto next_idx = cyclic_idx(current_idx, step, clips_size);
			const auto next_vert = clips[next_idx];
			if (next_vert.labeled != label_vert::label::in
				&& next_vert.labeled != label_vert::label::leaving_inter) {
				step *= - 1;
				continue;
			}
			if (!start_idx_found) {
				start_idx = current_idx;
				start_idx_found = true;
			}
			const auto is_dir_ccw_in_clip = step > 0 ? true : false;

			std::vector<label_vert> verts_to_exclude_clip;
			verts_to_exclude_clip.push_back(current_vert);

			const label_vert entering_intersect_in_clip = current_vert;
			label_vert leaving_intersect_in_clip = current_vert;
			auto clip_walk_idx = current_idx;
			while (true) {
				clip_walk_idx = cyclic_idx(clip_walk_idx, step, clips_size);
				const auto walk_vert = clips[clip_walk_idx];
				verts_to_exclude_clip.push_back(walk_vert);
				if (walk_vert.labeled == label_vert::label::leaving_inter) {
					leaving_intersect_in_clip = walk_vert;
					break;
				}
			}
			std::vector<label_vert> exclude_in_clip_sorted;
			if (is_dir_ccw_in_clip) {
				for (unsigned i = 0; i < verts_to_exclude_clip.size(); ++ i) {
					const auto exclude_vert = verts_to_exclude_clip[i];
					exclude_in_clip_sorted.push_back(exclude_vert);
				}
			} else {
				for (int i = static_cast<int>(verts_to_exclude_clip.size()) - 1; i >= 0; -- i) {
					const auto exclude_vert = verts_to_exclude_clip[static_cast<unsigned>(i)];
					exclude_in_clip_sorted.push_back(exclude_vert);
				}
			}

			// traverse subject list
			std::vector<label_vert> verts_to_exclude_subject;
			int subject_step = 1;
			unsigned subject_walk_idx = 0;
			while (true) {
				const auto walk_vert = subjects[subject_walk_idx];
				if (are_verts_same(walk_vert.vertex, leaving_intersect_in_clip.vertex)) {
					verts_to_exclude_subject.push_back(walk_vert);
					const auto next_idx = cyclic_idx(subject_walk_idx, subject_step, subjects.size());
					const auto next_vert = subjects[next_idx];
					if (next_vert.labeled == label_vert::label::out)
						subject_step *= -1;
					unsigned subject_inbound_walk_idx = subject_walk_idx;
					while (true) {
						subject_inbound_walk_idx = cyclic_idx(subject_inbound_walk_idx, subject_step, subjects.size());
						const auto subject_inbound_vert = subjects[subject_inbound_walk_idx];
						verts_to_exclude_subject.push_back(subject_inbound_vert);
						if (are_verts_same(subject_inbound_vert.vertex, entering_intersect_in_clip.vertex)) {
							break;
						}
					}
					break;
				}
				subject_walk_idx = cyclic_idx(subject_walk_idx, subject_step, subjects.size());
			}

			std::vector<label_vert> exclude;
			for (unsigned i = 0; i < exclude_in_clip_sorted.size(); ++ i)
				exclude.push_back(exclude_in_clip_sorted[i]);

			for (unsigned i = 1; i < verts_to_exclude_subject.size() - 1; ++ i)
				exclude.push_back(verts_to_exclude_subject[i]);

			const auto converted = sort_ccw(exclude);
			excluded_polys_.push_back(converted);

			if (exclude_in_clip_sorted.size() > 2) {
				int entering_pos = -1;
				int leaving_pos = -1;
				auto entering_it = clipped_poly_verts.begin();
				auto leaving_it = clipped_poly_verts.begin();
				int count = 0;
				for (auto it = clipped_poly_verts.begin(); it != clipped_poly_verts.end(); ++ it) {
					if (entering_pos >= 0 && leaving_pos >= 0) break;
					const auto clipped_poly_vert = (*it);
					if (are_verts_same(clipped_poly_vert.vertex, entering_intersect_in_clip.vertex)) {
						entering_it = it;
						entering_pos = count;
					} else if (are_verts_same(clipped_poly_vert.vertex, leaving_intersect_in_clip.vertex)) {
						leaving_it = it;
						leaving_pos = count;
					}
					++ count;
				}
				if (entering_pos < leaving_pos) {
					if (leaving_pos - entering_pos > 1) {
						auto remove_it_begin = entering_it;
						++ remove_it_begin;
						auto remove_it_end = leaving_it;
						-- remove_it_end;
						clipped_poly_verts.erase(remove_it_begin, remove_it_end);
					}

					const auto first_elem = verts_to_exclude_clip[0];
					const auto last_elem = verts_to_exclude_clip[verts_to_exclude_clip.size() - 1];
					if (are_verts_same(first_elem.vertex, leaving_intersect_in_clip.vertex)) {
						for (int i = static_cast<int>(verts_to_exclude_clip.size()) - 2; i >= 1; -- i) {
							const auto exclude_vert = verts_to_exclude_clip[static_cast<unsigned>(i)];
							clipped_poly_verts.insert(leaving_it, exclude_vert);
						}
					} else if (are_verts_same(last_elem.vertex, leaving_intersect_in_clip.vertex)) {
						for (unsigned i = 1; i + 1 < verts_to_exclude_clip.size(); ++ i) {
							const auto exclude_vert = verts_to_exclude_clip[i];
							clipped_poly_verts.insert(leaving_it, exclude_vert);
						}
					}
				} else {
					if (entering_pos - leaving_pos > 1) {
						auto remove_it_begin = leaving_it;
						++ remove_it_begin;
						auto remove_it_end = entering_it;
						-- remove_it_end;
						clipped_poly_verts.erase(remove_it_begin, remove_it_end);
					}

					const auto first_elem = verts_to_exclude_clip[0];
					const auto last_elem = verts_to_exclude_clip[verts_to_exclude_clip.size() - 1];
					if (are_verts_same(first_elem.vertex, entering_intersect_in_clip.vertex)) {
						for (int i = static_cast<int>(verts_to_exclude_clip.size()) - 2; i >= 1; -- i) {
							const auto exclude_vert = verts_to_exclude_clip[static_cast<unsigned>(i)];
							clipped_poly_verts.insert(entering_it, exclude_vert);
						}
					} else if (are_verts_same(last_elem.vertex, entering_intersect_in_clip.vertex)) {
						for (unsigned i = 1; i + 1 < verts_to_exclude_clip.size(); ++ i) {
							const auto exclude_vert = verts_to_exclude_clip[i];
							clipped_poly_verts.insert(entering_it, exclude_vert);
						}
					}
				}
			}
		}
	} while((!start_idx_found && current_idx < clips_size) || (start_idx_found && current_idx != start_idx));

	for (auto it = clipped_poly_verts.begin(); it != clipped_poly_verts.end(); ++ it)
		clipped_poly_.push_back((*it).vertex);
	clipped_tris_ = triangulate(clipped_poly_);

	return clipped_poly_.size() != 0 || excluded_polys_.size() != 0;
}

unsigned convex::cyclic_idx(const unsigned current_idx_,
				const int step_,
				const std::size_t size_) {
	assert(size_ != 0);
	const int next_idx = static_cast<int>(current_idx_) + step_;
	if (next_idx >= static_cast<int>(size_))
		return 0;
	else if (next_idx < 0)
		return static_cast<unsigned>(size_ - 1);
	else
		return static_cast<unsigned>(next_idx);
}

bool convex::are_verts_same(const glm::vec2 a_, const glm::vec2 b_) {
	const auto epsilon = glm::epsilon<float>();
	if ((glm::abs<float>(a_.x - b_.x) <= epsilon && glm::abs<float>(a_.y - b_.y) <= epsilon))
		return true;
	return false;
}

void convex::determine_vert(const unsigned prev_adj_idx_,
				const unsigned curr_idx_,
				const unsigned next_adj_idx_,
				const std::vector<glm::vec2> &point_set_,
				bool &is_convex_) {
	// check convex or reflex
	const auto vec_a = point_set_[prev_adj_idx_] - point_set_[curr_idx_];
	auto angle_a = std::atan2(vec_a.y, vec_a.x);
	const auto vec_b = point_set_[next_adj_idx_] - point_set_[curr_idx_];
	auto angle_b = std::atan2(vec_b.y, vec_b.x);
	auto angle_between = angle_b - angle_a;
	// https://stackoverflow.com/a/21484228
	const auto pi_2 = 2 * glm::pi<float>();
	if (angle_between < 0)
		angle_between += pi_2;

	if (angle_between > glm::pi<float>())
		is_convex_ = false;
	else
		is_convex_ = true;
}

std::vector<convex::label_vert> convex::sort_ccw(const std::vector<convex::label_vert> &point_set_) {
	const auto pn = point_set_.size();

	auto lowest = std::numeric_limits<float>::max();
	unsigned lowest_idx = static_cast<unsigned>(pn);
	for (unsigned i = 0; i < pn; ++ i) {
		if (const auto p_x = point_set_[i].vertex.x;
			p_x < lowest) {
			lowest = p_x;
			lowest_idx = i;
		}
	}

	if (lowest_idx == 0) return point_set_;

	std::vector<label_vert> sorted;
	auto walk_idx = lowest_idx;
	sorted.push_back(point_set_[walk_idx]);
	do {
		walk_idx = cyclic_idx(walk_idx, 1, pn);
		sorted.push_back(point_set_[walk_idx]);
	} while (walk_idx != lowest_idx);

	return sorted;
}

glm::vec2 convex::calc_centroid(std::vector<glm::vec2> verts_,
				std::vector<triangle> tris_) {
	// https://math.stackexchange.com/a/64328
	// https://stackoverflow.com/a/2832813
	auto total_centroid = glm::vec2(0, 0);
	for (unsigned i = 0; i < tris_.size(); ++ i) {
		auto tri = tris_[i];
		const auto vertices = tri.get_vertices(verts_);
		auto tri_centroid = glm::vec2(0.0f, 0.0f);
		for (unsigned i = 0; i < vertices.size(); ++ i)
			tri_centroid += vertices[i];
		tri_centroid /= static_cast<float>(vertices.size());
		total_centroid += tri_centroid;
	}
	total_centroid /= tris_.size();
	return total_centroid;
}