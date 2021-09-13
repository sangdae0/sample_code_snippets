#include <SDL.h>
#include "c_in_game.hh"

#include "c_math.hh"
#include "c_shader_mgr.hh"
#include "c_common.hh"
#include "c_input_mgr.hh"
#include "c_program_phong.hh"
#include "c_program_debug_shader.hh"
#include "c_collision.hh"
#include "c_bounding_volume.hh"

#include <iostream>

using namespace gam;
using namespace std;

c_in_game* c_in_game::m_inst = nullptr;

c_in_game* c_in_game::get() {
	if (m_inst == nullptr)
		m_inst = new c_in_game();
	return m_inst;
}

void c_in_game::terminate() {
	if (m_inst != nullptr) {
		delete m_inst;
		m_inst = nullptr;
	}
}

c_in_game::~c_in_game() {
	destroy();
}

void c_in_game::create() {
	c_shader_mgr::get()->m_program_debug_shader->init();
	c_shader_mgr::get()->m_program_phong->init();

	m_phong_prop_global_ambient_intensity = Vector3f(0.1f, 0.1f, 0.1f);
	m_phong_prop_fog_intensity = Vector3f(0.0f, 0.0f, 0.0f);
	m_phong_prop_current_light_count = 0;
	m_phong_prop_z_near = 1.0f;
	m_phong_prop_z_far = 300.0f;

	Vector3f lower_crevasse_pos = Vector3f(150.0f, 0.0f, 150.0f);
	m_needleleaf_tree_pos = lower_crevasse_pos;
	
	float lower_crevasse_height = 150.0f;
	float lower_crevasse_radius = 70.0f;

	Vector3f upper_crevasse_pos = Vector3f(lower_crevasse_pos.x(),
											lower_crevasse_pos.y() + lower_crevasse_height,
											lower_crevasse_pos.z());
	float upper_crevasse_radius = 25.0f;
	float upper_crevasse_height = 150.0f;

	m_camera = new c_camera();
	Vector3f cam_target_pos = lower_crevasse_pos;
	Vector3f eye_pos = lower_crevasse_pos + (Vector3f::UnitZ() * 25.0f);

	Vector3f cam_to_target_dir = (cam_target_pos - eye_pos).normalized();
	m_camera->init(eye_pos, // eye
					cam_to_target_dir, // target
					Vector3f::UnitY(), // up
					false);

	Vector3f head_light_color = Vector3f(1.0f, 1.0f, 0.7f);
	// https://learnopengl.com/Lighting/Light-casters
	m_head_light = c_spot_light::gen(m_camera->m_eye,
										m_camera->m_forward_dir,
										head_light_color,
										head_light_color,
										head_light_color,
										1.0f, 0.007f, 0.0002f,
										5.0f, 70.0f, 1.1f);
	add_spot_light(m_head_light);

	// 25.0f is radius
	// global light in lower crevasse
	auto moon_pos = lower_crevasse_pos + (Vector3f::UnitY() * (lower_crevasse_height + 50.0f));
	const Vector3f moon_light_color = Vector3f(0.6f, 0.6f, 1.0f);
	add_spot_light(c_spot_light::gen(moon_pos,
										-Vector3f::UnitY(),
										moon_light_color,
										moon_light_color,
										moon_light_color,
										1.0f, 0.0001f, 0.00001f,
										0.1f, 15.0f, 1.15f));
	//add_directional_light(c_directional_light::gen(-Vector3f::UnitY(),
	//									moon_light_color,
	//									moon_light_color,
	//									moon_light_color));

	// add star lights
	int star_count = 5;
	float theta_step = (2.0f * c_math::pi()) / star_count;
	float star_radius = lower_crevasse_radius - 5.0f;
	for (int i = 0; i < star_count; ++ i) {
		auto unit_pos = Vector3f(cos(i * theta_step) * star_radius,
									0,
									sin(i * theta_step) * star_radius);
		auto star_lt_pos = moon_pos + unit_pos;
		auto star_lt_dir = (lower_crevasse_pos - star_lt_pos).normalized();
		//star_lt_dir = -Vector3f::UnitY();

		//add_spot_light(c_spot_light::gen(star_lt_pos,
		//									star_lt_dir,
		//									Vector3f(0.1f, 0.1f, 0.1f),
		//									Vector3f(0.1f, 0.1f, 0.1f),
		//									Vector3f(0.1f, 0.1f, 0.1f),
		//									1.0f, 0.0001f, 0.00001f,
		//									0.1f, 45.0f, 1.15f));
	}

	auto terrain_size = 700.0f;
	auto terrain_row_tile_count = 10;
	auto terrain_col_tile_count = 10;
	auto terrain_start_pos = Vector3f(0, 0, 0);

	float terrain_half_size = terrain_size * 0.5f;
	Vector3f terrain_origin = terrain_start_pos
							+ (Vector3f::UnitX() * (terrain_size * 0.5f))
							+ (Vector3f::UnitZ() * (terrain_size * 0.5f));
	m_terrain_origin = terrain_origin;

	m_world_oct = new c_oct_tree(terrain_origin,
									terrain_half_size,
									100,
									5);
								
	
	float terrain_ns = 32.0f;
	auto terrain = c_object::gen_with_mesh_builder(
							c_mesh_builder::gen_terrain(nullptr,
														terrain_half_size,
														terrain_half_size,
														terrain_row_tile_count,
														terrain_col_tile_count,
														terrain_start_pos),
							c_material::gen(Vector3f(0.0f, 0.0f, 0.0f), // ambient
											Vector3f(1.0f, 1.0f, 1.0f), // diffuse
											Vector3f(0.0f, 0.0f, 0.0f), // specular
											Vector3f(0.0f, 0.0f, 0.0f), // emission
											terrain_ns, // shininess
											"../resources/texture/terrain.png"),
							tex_mapping_type::planar_positive_y_axis);

	terrain->m_collidables.push_back(new c_plane(Vector3f::UnitY(),
											-Vector3f::UnitZ(),
											terrain_origin,
											m_maximum_bullet_plane_thickness,
											true,
											terrain_size,
											terrain_size));
	terrain->m_terrain_cmp = new c_terrain_cmp();

	terrain->m_terrain_cmp->m_boundary_min_x = terrain_start_pos.x();
	terrain->m_terrain_cmp->m_boundary_min_z = terrain_start_pos.z();

	terrain->m_terrain_cmp->m_boundary_max_x = terrain->m_terrain_cmp->m_boundary_min_x
												+ terrain_size;
	terrain->m_terrain_cmp->m_boundary_max_z = terrain->m_terrain_cmp->m_boundary_min_z
												+ terrain_size;

	const float ring_out_boundary_vertical_offset = 100.0f;
	m_ring_out_y = terrain_start_pos.y() - ring_out_boundary_vertical_offset;
	const float ring_out_boundary_horizontal_offset = 100.0f;
	m_ring_out_min_x = terrain->m_terrain_cmp->m_boundary_min_x - ring_out_boundary_horizontal_offset;
	m_ring_out_min_z = terrain->m_terrain_cmp->m_boundary_min_z - ring_out_boundary_horizontal_offset;
	m_ring_out_max_x = terrain->m_terrain_cmp->m_boundary_max_x + ring_out_boundary_horizontal_offset;
	m_ring_out_max_z = terrain->m_terrain_cmp->m_boundary_max_z + ring_out_boundary_horizontal_offset;

	m_terrain = terrain;
	auto terrain_bv = new c_bounding_volume(terrain);
	m_world_oct->insert(terrain_bv);


	float crevasse_wall_ns = 10.0f;
	auto crevasse_wall_material = c_material::gen(Vector3f(0.1f, 0.1f, 0.1f), // ambient
													Vector3f(1.0f, 1.0f, 1.0f), // diffuse
													Vector3f(1.0f, 1.0f, 1.0f), // specular
													Vector3f(0.0f, 0.0f, 0.01f), // emission
													crevasse_wall_ns,
													"../resources/texture/ice_wall.png");
	int ice_wall_count = 20;
	// set ice walls
	float wall_theta_step = (2.0f * c_math::pi()) / ice_wall_count;
													
	float wall_width = (Vector3f(cos(0 * wall_theta_step) * lower_crevasse_radius, 0, sin(0 * wall_theta_step) * lower_crevasse_radius)
						- Vector3f(cos(1 * wall_theta_step) * lower_crevasse_radius, 0, sin(1 * wall_theta_step) * lower_crevasse_radius))
						.norm(); // use wall width as length vector of prev ring vert and next ring vert
	float wall_height = (lower_crevasse_pos + (Vector3f::UnitY() * lower_crevasse_height)).y() - lower_crevasse_pos.y();

	float wall_half_width = wall_width * 0.5f;
	float wall_half_height = wall_height * 0.5f;


	const float bullet_material_ns = 64.0f;
	auto bullet_material = c_material::gen(Vector3f(0.0f, 0.0f, 0.0f),
											Vector3f(1.0f, 1.0f, 1.0f), // https://www.htmlcsscolor.com/hex/2A2A2A
											Vector3f(1.0f, 1.0f, 1.0f),
											Vector3f(0.0f, 0.0f, 0.0f),
											bullet_material_ns,
											"../resources/texture/aluminum.png");



	float wall_thickness = c_in_game::get()->m_maximum_bullet_plane_thickness;
	Vector3f center_unit_pos = lower_crevasse_pos;
	
	for (int i = 0; i < ice_wall_count + 1; ++ i) {
		int current_idx = i;
		int next_idx = (i + 1) % ice_wall_count;
		auto current_unit_pos = Vector3f(cos(current_idx * wall_theta_step) * lower_crevasse_radius,
											0,
											sin(current_idx * wall_theta_step) * lower_crevasse_radius);
		auto next_unit_pos = Vector3f(cos(next_idx * wall_theta_step) * lower_crevasse_radius,
											0,
											sin(next_idx * wall_theta_step) * lower_crevasse_radius);

		auto to_next_unit_pos = next_unit_pos - current_unit_pos;
		auto middle_unit_pos = current_unit_pos + (to_next_unit_pos * 0.5f);

		auto to_next_unit_pos_dir = to_next_unit_pos.normalized();

		if (i < ice_wall_count) {
			float platform_ns = 32.0f;
			auto plane_normal_dir = Vector3f::UnitY();
			auto middle_unit_pos = current_unit_pos + (to_next_unit_pos * 0.20f);
			Vector3f plane_origin_pos = lower_crevasse_pos + (middle_unit_pos + (Vector3f::UnitY() * i * 10.0f));

			auto platform_obj = c_object::gen_with_mesh_builder(
										c_mesh_builder::gen_cube(nullptr,
																	Vector3f::Zero(),
																	Vector3f(10.0f, 10.0f, 2.0f)),
										bullet_material,
										tex_mapping_type::planar_positive_y_axis);
			Vector3f wall_rot_euler_rad = Quaternionf::FromTwoVectors(Vector3f::UnitZ(), plane_normal_dir)
											.toRotationMatrix().eulerAngles(0, 1, 2);
			platform_obj->m_euler_angle = Vector3f(c_math::rad_to_deg(wall_rot_euler_rad.x()),
																c_math::rad_to_deg(wall_rot_euler_rad.y()),
																c_math::rad_to_deg(wall_rot_euler_rad.z()));

			platform_obj->set_pos(plane_origin_pos);
			m_other_objects.push_back(platform_obj);

			platform_obj->m_collidables.push_back(new c_plane(plane_normal_dir,
																Vector3f::UnitY(),
																plane_origin_pos,
																2.0f,
																true,
																5.0f,
																5.0f));
			auto platform_bv = new c_bounding_volume(platform_obj);
			m_world_oct->insert(platform_bv);
		}


		// ceiling
		if (i == ice_wall_count) {
			auto plane_normal_dir = -Vector3f::UnitY();
		
			Vector3f plane_origin_pos = lower_crevasse_pos + Vector3f::UnitY() * lower_crevasse_height;

			auto lower_crevasse_wall_obj = c_object::gen_with_mesh_builder(
										c_mesh_builder::gen_half_quad(nullptr,
																	75.0f,
																	75.0f),
										crevasse_wall_material,
										tex_mapping_type::planar_negative_y_axis);
			Vector3f wall_rot_euler_rad = Quaternionf::FromTwoVectors(Vector3f::UnitZ(), plane_normal_dir)
											.toRotationMatrix().eulerAngles(0, 1, 2);
			lower_crevasse_wall_obj->m_euler_angle = Vector3f(c_math::rad_to_deg(wall_rot_euler_rad.x()),
																c_math::rad_to_deg(wall_rot_euler_rad.y()),
																c_math::rad_to_deg(wall_rot_euler_rad.z()));

			lower_crevasse_wall_obj->set_pos(plane_origin_pos);
			m_crevasses.push_back(lower_crevasse_wall_obj);

			lower_crevasse_wall_obj->m_collidables.push_back(new c_plane(plane_normal_dir,
																-Vector3f::UnitY(),
																plane_origin_pos,
																wall_thickness,
																true,
																150.0f,
																150.0f));
			auto lower_crevasse_bv = new c_bounding_volume(lower_crevasse_wall_obj);
			m_world_oct->insert(lower_crevasse_bv);
		} else {
			auto plane_normal_dir = to_next_unit_pos_dir.cross(Vector3f::UnitY()).normalized();
		
			Vector3f plane_origin_pos = lower_crevasse_pos + (middle_unit_pos + (Vector3f::UnitY() * wall_half_height));

			auto lower_crevasse_wall_obj = c_object::gen_with_mesh_builder(
										c_mesh_builder::gen_half_quad(nullptr,
																	wall_half_width,
																	wall_half_height),
										crevasse_wall_material,
										tex_mapping_type::planar_positive_z_axis);
			Vector3f wall_rot_euler_rad = Quaternionf::FromTwoVectors(Vector3f::UnitZ(), plane_normal_dir)
											.toRotationMatrix().eulerAngles(0, 1, 2);
			lower_crevasse_wall_obj->m_euler_angle = Vector3f(c_math::rad_to_deg(wall_rot_euler_rad.x()),
																c_math::rad_to_deg(wall_rot_euler_rad.y()),
																c_math::rad_to_deg(wall_rot_euler_rad.z()));

			lower_crevasse_wall_obj->set_pos(plane_origin_pos);
			m_crevasses.push_back(lower_crevasse_wall_obj);

			lower_crevasse_wall_obj->m_collidables.push_back(new c_plane(plane_normal_dir,
																Vector3f::UnitY(),
																plane_origin_pos,
																wall_thickness,
																true,
																wall_width,
																wall_height));
			auto lower_crevasse_bv = new c_bounding_volume(lower_crevasse_wall_obj);
			m_world_oct->insert(lower_crevasse_bv);
		}
	}

	//int lower_crevasse_slice_count = 20;

	//auto lower_crevasse = c_object::gen_with_mesh_builder(
	//							c_mesh_builder::gen_cylinder(nullptr,
	//															Vector3f::Zero(),
	//															lower_crevasse_height,
	//															lower_crevasse_radius,
	//															lower_crevasse_radius,
	//															lower_crevasse_slice_count,
	//															10,
	//															false,
	//															Vector3f::Zero(),
	//															true,
	//															true,
	//															true),
	//							crevasse_material);
	//lower_crevasse->set_pos(lower_crevasse_pos);
	//lower_crevasse->m_crevasse_cmp = new c_crevasse_cmp();
	//lower_crevasse->m_crevasse_cmp->set_ice_walls(lower_crevasse,
	//												lower_crevasse_pos,
	//												lower_crevasse_pos + (Vector3f::UnitY() * lower_crevasse_height),
	//												lower_crevasse_radius,
	//												lower_crevasse_collision_slice_count);
	//m_crevasses.push_back(lower_crevasse);
	//auto lower_crevasse_bv = new c_bounding_volume(lower_crevasse);
	//m_world_oct->insert(lower_crevasse_bv);

	auto upper_crevasse_material = c_material::gen(Vector3f(0.5f, 0.5f, 0.5f), // ambient
												Vector3f(1.0f, 1.0f, 1.0f), // diffuse
												Vector3f(1.0f, 1.0f, 1.0f), // specular
												Vector3f(0.0f, 0.0f, 0.0f), // emission
												10.0f);
	auto upper_crevasse = c_object::gen_with_mesh_builder(
								c_mesh_builder::gen_cylinder(nullptr,
																Vector3f::Zero(),
																upper_crevasse_height,
																upper_crevasse_radius,
																upper_crevasse_radius,
																20,
																10,
																false,
																Vector3f::Zero(),
																false,
																true,
																true),
								upper_crevasse_material);
	upper_crevasse->set_pos(upper_crevasse_pos - (Vector3f::UnitY() * 20.0f));
	m_crevasses.push_back(upper_crevasse);
	auto upper_crevasse_bv = new c_bounding_volume(upper_crevasse);
	//m_world_oct->insert(upper_crevasse_bv);

	// will not render
	auto player_material = c_material::gen(Vector3f(0.0f, 0.0f, 0.0f), // ambient
												Vector3f(0.0f, 0.0f, 0.0f), // diffuse
												Vector3f(0.0f, 0.0f, 0.0f), // specular
												Vector3f(0.0f, 0.0f, 0.0f), // emission
												0.0f);
	c_object* player = new c_object(eye_pos);
	player->m_player_cmp = new c_player_cmp(player);
	float player_foot_radius = 2.5f;
	player->m_collidables.push_back(new c_sphere(Vector3f::Zero(),
													player_foot_radius,
													true));
	m_player = player;


	// create needleaf_group
	{
		auto needleleaf_group_tree = new c_tree_group();
		needleleaf_group_tree->m_trunk = c_plant_tree::gen_from_file("trunk",
																			"../data/needleleaf_trunk.txt");
		needleleaf_group_tree->m_branch_gen_file = "../data/needleleaf_branch.txt";
		needleleaf_group_tree->m_root_scale = Vector3f(1.0f, 1.0f, 1.0f);
		needleleaf_group_tree->m_root_pos = m_needleleaf_tree_pos;
		needleleaf_group_tree->start();
		m_tree_groups.push_back(needleleaf_group_tree);
	}
}

void c_in_game::start() {
}

void c_in_game::update(float dt_,
						int main_window_width_,
						int main_window_height_) {
	if (c_input_mgr::get()->m_kb_tab_pressed)
		m_render_debug = true;
	else
		m_render_debug = false;

	if (!m_camera)
		return;

	auto view_mat = m_camera->get_view_mat();
	auto proj_mat = m_camera->get_perspective_proj_mat(85.0,
														static_cast<float>(main_window_width_) / main_window_height_,
														m_phong_prop_z_near,
														m_phong_prop_z_far);

	if (m_player) {
		m_player->update(dt_);
		if (c_input_mgr::get()->m_mc_is_left_triggered) {
			c_object* fired_bullet = m_player
										->m_player_cmp
											->fire_bullet(m_player,
															m_camera->m_forward_dir,
															m_camera->m_horizontal_angle,
															m_camera->m_vertical_angle);
			if (fired_bullet != nullptr)
				m_bullets.push_back(fired_bullet);
		}
	}

	if (m_terrain)
		m_terrain->update(dt_);

	for (unsigned i = 0; i < m_bullets.size(); ++ i) {
		auto bullet = m_bullets[i];
		if (!bullet
			|| bullet->m_destroy_requested
			|| is_ring_out(bullet->get_pos())) {
			delete bullet;
			m_bullets.erase(m_bullets.begin() + i);
		} else {
			bullet->update(dt_);
		}
	}

	for (unsigned i = 0; i < m_crevasses.size(); ++ i) {
		auto crevasse = m_crevasses[i];
		if (crevasse)
			crevasse->update(dt_);
	}


	for (unsigned i = 0; i < m_tree_groups.size(); ++ i) {
		auto tree_group = m_tree_groups[i];
		if (tree_group) {
			if (c_input_mgr::get()->m_kb_enter_triggered) {
				tree_group->increment_time_by(100.0f * dt_);
			}
			tree_group->update(dt_);
		}
	}

	for (int i = static_cast<int>(m_other_objects.size()) - 1; i >= 0; -- i) {
		auto object = m_other_objects[i];
		if (object == nullptr)
			continue;
		if (object->m_destroy_requested
			|| is_ring_out(object->get_pos())) {
			delete object;
			m_other_objects.erase(m_other_objects.begin() + i);
		} else {
			object->update(dt_);
		}
	}
	
// num_of_iterations: (n * (n - 1)) / 2 -> n^2
	process_collision(dt_, view_mat, proj_mat);



	// collision event call maximum count:
	// 2 * num of iterations of collision checking
	// = n * (n - 1) -> n^2

	if (m_player && !m_player->m_destroy_requested) {
		for (unsigned i = 0; i < m_player->m_last_collision_events.size(); ++ i)
			m_player->process_collision_event(m_player->m_last_collision_events[i]);
		m_player->m_last_collision_events.clear();
	}

	for (unsigned i = 0; i < m_bullets.size(); ++ i) {
		auto bullet = m_bullets[i];
		if (!bullet || bullet->m_collidables.size() == 0 || bullet->m_destroy_requested)
			continue;
		for (unsigned j = 0; j < bullet->m_last_collision_events.size(); ++ j)
			bullet->process_collision_event(bullet->m_last_collision_events[j]);
		bullet->m_last_collision_events.clear();
	}

	// no collision response need for
	// wind blowers and light emitters

	c_shader_mgr::get()->m_program_phong->update_phong_props(m_phong_prop_global_ambient_intensity,
														m_phong_prop_fog_intensity,
														m_phong_prop_current_light_count,
														m_phong_prop_z_near,
														m_phong_prop_z_far);
	if (m_head_light && m_camera) {
		m_head_light->m_lt_pos = m_camera->m_eye;
		m_head_light->m_lt_dir = m_camera->m_forward_dir;
	}
	bool rotate_lights = true;
	float rot_speed = 50.0;
	for (unsigned i = 0; i < static_cast<unsigned>(m_phong_prop_current_light_count); ++ i) {
		auto lt = m_lights[i];
		if (lt == nullptr)
			continue;
		if (lt->m_lt_type == 0) { // directional light
			//auto as_dir_lt = dynamic_cast<c_directional_light*>(lt);
			//if (rotate_lights == true) {
			//	auto rot_mat = c_math::mat4_get_rotated_y(c_math::deg_to_rad(rot_speed * dt_));
			//	as_dir_lt->m_lt_dir = c_math::to_vec3(rot_mat * c_math::to_vec4(as_dir_lt->m_lt_dir, 0.0));
			//}
		} else if (lt->m_lt_type == 1) { // point light
			//if (m_is_debug_build) {
			//	auto point_lt = dynamic_cast<c_point_light*>(lt);
			//	if (point_lt && point_lt->m_debug_render_sphere)
			//		point_lt->m_debug_render_sphere->update_bv( = point_lt->m_lt_pos;
			//}
		} else if (lt->m_lt_type == 2) { // spot light
			//if (m_is_debug_build) {
			//	auto spot_lt = dynamic_cast<c_spot_light*>(lt);

			//	if (spot_lt && spot_lt->m_debug_render_sphere_start) {
			//		spot_lt->m_debug_render_sphere_start->m_last_updated_center = spot_lt->m_lt_pos;
			//		spot_lt->m_debug_render_sphere_end->m_last_updated_center = spot_lt->m_lt_pos + spot_lt->m_lt_dir;
			//	}
			//}
		}
		c_shader_mgr::get()->m_program_phong->update_specific_light(i, lt, view_mat);
	}
	c_shader_mgr::get()->m_program_phong->use();
	c_shader_mgr::get()->m_program_phong->update_lights_ubo();

	// https://developer.mozilla.org/en-US/docs/Web/API/WebGL_API/Tutorial/Adding_2D_content_to_a_WebGL_context
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // Near things obscure far things
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	glViewport(0, 0, main_window_width_, main_window_height_);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (!m_render_debug)
		render(dt_, view_mat, proj_mat);
	debug_render(dt_, view_mat, proj_mat);
}

void c_in_game::process_collision(float dt_,
									Matrix4f view_mat_,
									Matrix4f proj_mat_) {
	if (!m_world_oct)
		return;

	// bullet vs bullet
	// bullet vs wind blower
	// bullet vs player
	// bullet vs static bvs oct tree
		// bullet vs screw
		// bullet vs platform 
		// bullet vs tree
		// bullet vs crevasse
		// bullet vs terrain

	// player vs pickups(non grid or oct tree)
	// player vs wind blower
	// player vs static bvs oct tree
		// player vs screw
		// player vs platform 
		// player vs tree
		// player vs crevasse
		// player vs terrain
	
	// wind blower(cone) vs tree
	// wind blower(cone) vs screw
	
	// light src(cone) vs tree
	// light src(cone) vs screw

	// not in loop
		// tree(or screw) vs screw(not growing) -> raycast
		// tree(or screw) vs platform(not growing) -> raycast
		// tree(or screw) vs tree(not growing) -> raycast
		// tree(or screw) vs terrain(not growing) -> raycast
		// tree(or screw) vs crevasse(dead) -> raycast
	int debug_collision_check_count = 0;
	int debug_iterate_count = 0;

	bool ignore_player = !m_player
							|| m_player->m_collidables.size() == 0
							|| m_player->m_destroy_requested;

	// num_of_iterations: (n * (n - 1)) / 2 -> n^2
	int bullet_count = static_cast<int>(m_bullets.size());
	int wind_blower_count = static_cast<int>(m_wind_blowers.size());
	int crevasse_count = static_cast<int>(m_crevasses.size());
	for (int i = 0; i < bullet_count; ++ i) {
		++ debug_iterate_count;
		auto bullet_a = m_bullets[i];
		if (!bullet_a
			|| bullet_a->m_collidables.size() == 0
			|| bullet_a->m_destroy_requested)
			continue;

		auto bullet_hit_sphere = dynamic_cast<c_sphere*>(bullet_a->m_collidables[0]);
		if (!bullet_hit_sphere)
			continue;

		auto bullet_a_pos = bullet_hit_sphere->get_pos();
		auto bullet_a_radius = bullet_hit_sphere->get_radius();

		// bullet vs wind blower
		for (int j = 0; j < wind_blower_count; ++ j) {
			++ debug_iterate_count;
			++ debug_collision_check_count;

			auto wind_blower = m_wind_blowers[j];
			if (!wind_blower
				|| wind_blower->m_collidables.size() == 0
				|| wind_blower->m_destroy_requested)
				continue;
			calc_collision_events(bullet_a, wind_blower, dt_);
		}

		for (int j = 0; j < m_tree_groups.size(); ++ j) {
			auto tree_group = m_tree_groups[j];
			if (!tree_group)
				continue;
			auto trunk = tree_group->m_trunk;
			if (!trunk || !trunk->m_root_tree)
				continue;
			Vector3f collided_pos = Vector3f::Zero();
			float collided_sphere_radius = 0.0f;
			c_object* any_collided = trunk->check_collision_sphere(bullet_a_pos,
																		bullet_a_radius,
																		collided_pos,
																		collided_sphere_radius);
			if (any_collided) {
				trunk->when_got_hit(any_collided, dt_);
			}
		}

		// bullet vs player
		++ debug_collision_check_count;
		if (!ignore_player) {
			calc_collision_events(bullet_a, m_player, dt_);
		}

		for (int j = 0; j < crevasse_count; ++ j) {
			++ debug_iterate_count;
			++ debug_collision_check_count;

			auto crevasse = m_crevasses[j];
			if (!crevasse
				|| crevasse->m_collidables.size() == 0
				|| crevasse->m_destroy_requested)
				continue;
			calc_collision_events(bullet_a, crevasse, dt_);
		}

		if (m_terrain) {
			++ debug_collision_check_count;
			calc_collision_events(bullet_a, m_terrain, dt_);
		}

		//// raycast to
		//if (bullet_a-> raycast to oct_tree) {
		//	++ debug_collision_check_count;
		//	// bullet vs static bvs oct tree
		//	// bullet vs screw
		//	// bullet vs platform 
		//	// bullet vs tree
		//	calc_collision_events();
		//}
	}

	if (!ignore_player) {
		auto player_sphere = dynamic_cast<c_sphere*>(m_player->m_collidables[0]);
		auto player_pos = player_sphere->get_pos();
		auto player_radius = player_sphere->get_radius();

		for (int j = 0; j < wind_blower_count; ++ j) {
			++ debug_iterate_count;
			++ debug_collision_check_count;

			auto wind_blower = m_wind_blowers[j];
			if (!wind_blower
				|| wind_blower->m_collidables.size() == 0
				|| wind_blower->m_destroy_requested)
				continue;
			calc_collision_events(m_player, wind_blower, dt_);
		}

		for (int j = 0; j < crevasse_count; ++ j) {
			++ debug_iterate_count;
			++ debug_collision_check_count;

			auto crevasse = m_crevasses[j];
			if (!crevasse
				|| crevasse->m_collidables.size() == 0
				|| crevasse->m_destroy_requested)
				continue;
			calc_collision_events(m_player, crevasse, dt_);
		}

		//bool any_found = false;
		//float check_y = numeric_limits<float>::lowest();
		//for (int j = 0; j < m_other_objects.size(); ++ j) {
		//	++ debug_iterate_count;
		//	++ debug_collision_check_count;

		//	auto other_object = m_other_objects[j];
		//	if (!other_object
		//		|| other_object->m_collidables.size() == 0
		//		|| other_object->m_destroy_requested)
		//		continue;

		//	auto platform_plane = dynamic_cast<c_plane*>(other_object->m_collidables[0]);
		//	if (!platform_plane)
		//		continue;
		//	auto ret = c_collision::check_single_face_plane_with_boundaries_vs_sphere(platform_plane->m_normal_dir,
		//													platform_plane->m_normal_up_dir,
		//													platform_plane->m_origin,
		//													5.0f,
		//													5.0f,
		//													player_sphere,
		//													2.0f);

		//	if (ret) {
		//		any_found = true;
		//		if (platform_plane->m_origin.y() > check_y) {
		//			check_y = platform_plane->m_origin.y();
		//		}
		//	}
		//}

		//if (any_found) {
		//	m_player->m_player_cmp->m_current_min_y =check_y;// -  m_player->m_player_cmp->m_min_y_offset;
		//} else {
		//	m_player->m_player_cmp->m_current_min_y = nullopt;
		//}

		bool player_collided_with_tree = false;
		//for (int j = 0; j < m_plant_trees.size(); ++ j) {
		//	auto plant_tree = m_plant_trees[j];
		//	if (!plant_tree)
		//		continue;
		//	Vector3f collided_pos = Vector3f::Zero();
		//	float collided_sphere_radius = 0.0f;
		//	c_object* any_collided = plant_tree->check_collision_sphere(player_pos,
		//																player_radius,
		//																collided_pos,
		//																collided_sphere_radius);
		//	if (any_collided) {
		//		player_collided_with_tree = true;
		//		cout << "player collided with tree!" << endl;
		//		m_player->m_player_cmp->m_current_min_y = (collided_pos 
		//														+ Vector3f::UnitY() * collided_sphere_radius).y()
		//														-  m_player->m_player_cmp->m_min_y_offset;
		//	}
		//}
		if (!player_collided_with_tree) {
			m_player->m_player_cmp->m_current_min_y = nullopt;
		}

		//float ray_t = -1.0f;
		//Vector3f cast_pos = m_player->get_pos();
		//Vector3f cast_dir = -Vector3f::UnitY();
		//if (query_raycast(cast_pos,
		//					cast_dir,
		//					ray_t)) {
		//	if (ray_t >= 0.0f) {
		//		Vector3f picked_pos = cast_pos + (cast_dir * ray_t);
		//		m_player->set_pos(Vector3f(cast_pos.x(),
		//									picked_pos.y(),
		//									cast_pos.z()));
		//		cout << "test: " << ray_t << endl;
		//	}
		//}

		//if (player-> raycast to oct_tree) {
		//	++ debug_collision_check_count;
		//	// player vs static bvs oct tree
		//	// player vs screw
		//	// player vs platform 
		//	// player vs tree
		//	// player vs crevasse
		//	// player vs terrain
		//	calc_collision_events();
		//}
	}

	// wind blower(cone) vs tree
	// wind blower(cone) vs screw
	for (int i = 0; i < wind_blower_count; ++ i) {
		++ debug_iterate_count;
		++ debug_collision_check_count;

		auto wind_blower = m_wind_blowers[i];
		if (!wind_blower
			|| wind_blower->m_collidables.size() == 0
			|| wind_blower->m_destroy_requested)
			continue;
		//if (wind_blower-> raycast to oct_tree) {
		//	calc_collision_events();
		//}
	}

	int light_emitter_count = static_cast<int>(m_light_emitters.size());
	for (int i = 0; i < light_emitter_count; ++ i) {
		++ debug_iterate_count;
		++ debug_collision_check_count;

		auto light_emitter = m_light_emitters[i];
		if (!light_emitter
			|| light_emitter->m_collidables.size() == 0
			|| light_emitter->m_destroy_requested)
			continue;
		//if (light_emitter-> raycast to oct_tree) {
		//	calc_collision_events();
		//}
	}


	// collision event call maximum count:
	// 2 * num of iterations of collision checking
	// = n * (n - 1) -> n^2
	//for (int i = 0; i < ; ++ i) {
	//	auto obj = m_objects[i];
	//	if (!obj || obj->m_collidables.size() == 0 || obj->m_destroy_requested)
	//		continue;
	//	else if (obj->m_last_collision_events.size() == 0)
	//		continue;
	//	for (unsigned j = 0; j < obj->m_last_collision_events.size(); ++ j)
	//		obj->process_collision_event(obj->m_last_collision_events[j]);
	//	obj->m_last_collision_events.clear();
	//}

		// terrains and crevasses and trees are in oct tree
	//vector<c_oct_tree
	//// from Ericson, C. Real-Time Collision Detection  p.313
	//for (int i = 0; i < m_world_oct->depth; ++ i) {
	//	c_oct_tree::c_node* current = nullptr;
	//	for (unsigned j = 0; j < current->m_bvs.size(); ++ j) {
	//		auto bv_a = current->m_bvs[j];
	//		if (!bv_a == 0 || bv_a->
	//	auto obj_a = m_objects[i];
	//	if (!obj_a || obj_a->m_collidables.size() == 0 || obj_a->m_destroy_requested)
	//		continue;
	//	for (int j = i + 1; j < obj_count; ++ j) {
	//		auto obj_b = m_objects[j];
	//		if (!obj_b || obj_b->m_collidables.size() == 0)
	//			continue;
	//		calc_collision_events(obj_a, obj_b, dt_);
	//	}

	//	}
	//}
}

void c_in_game::calc_collision_events(c_object* a_, c_object* b_, float dt_) {
	if ((a_->m_player_cmp && b_->m_terrain_cmp) // player vs terrain
		|| (a_->m_terrain_cmp && b_->m_player_cmp)) {
		c_object* player = nullptr;
		c_object* terrain = nullptr;
		if (a_->m_player_cmp) {
			player = a_;
			terrain = b_;
		} else {
			player = b_;
			terrain = a_;
		}

		auto player_sphere = dynamic_cast<c_sphere*>(player->m_collidables[0]);
		// check player pos is in terrain's boundary
		Vector3f player_pos = player->get_pos() + (player->m_player_cmp->m_vel * dt_);
		float player_sphere_radius = player_sphere->get_radius();
		if (((player_pos.x() + player_sphere_radius) 
				< terrain->m_terrain_cmp->m_boundary_min_x)
			|| ((player_pos.z() + player_sphere_radius)
				< terrain->m_terrain_cmp->m_boundary_min_z)
			|| ((player_pos.x() - player_sphere_radius)
				> terrain->m_terrain_cmp->m_boundary_max_x)
			|| ((player_pos.z() - player_sphere_radius)
				> terrain->m_terrain_cmp->m_boundary_max_z))
			return;
		auto terrain_plane = dynamic_cast<c_plane*>(terrain->m_collidables[0]);

		if (!c_collision::check_single_face_plane_with_boundaries_vs_sphere(terrain_plane->m_normal_dir,
																			terrain_plane->m_normal_up_dir,
																			terrain_plane->m_origin,
																			terrain_plane->m_drawing_width,
																			terrain_plane->m_drawing_height,
																			player_sphere,
																			terrain_plane->m_thickness))
			return;
		terrain->m_last_collision_events.push_back(c_collision_event(player,
																		Vector3f::Zero()));
		player->m_last_collision_events.push_back(c_collision_event(terrain,
																		Vector3f::Zero()));
		
	} else if ((a_->m_player_cmp && b_->m_crevasse_cmp) // player vs crevasse
		|| (a_->m_crevasse_cmp && b_->m_player_cmp)) {
		c_object* player = nullptr;
		c_object* crevasse = nullptr;
		if (a_->m_player_cmp) {
			player = a_;
			crevasse = b_;
		} else {
			player = b_;
			crevasse = a_;
		}

		auto player_sphere = dynamic_cast<c_sphere*>(player->m_collidables[0]);

		// todo optimize with bb
		for (unsigned i = 0; i < crevasse->m_collidables.size(); ++ i) {
			auto wall = crevasse->m_collidables[i];
			auto wall_as_plane = dynamic_cast<c_plane*>(wall);
			if (!c_collision::check_single_face_plane_with_boundaries_vs_sphere(wall_as_plane->m_normal_dir,
																				wall_as_plane->m_normal_up_dir,
																				wall_as_plane->m_origin,
																				wall_as_plane->m_drawing_width,
																				wall_as_plane->m_drawing_height,
																				player_sphere,
																				wall_as_plane->m_thickness))
				continue;
			crevasse->m_last_collision_events.push_back(c_collision_event(player,
																			Vector3f::Zero()));
			player->m_last_collision_events.push_back(c_collision_event(crevasse,
																			Vector3f::Zero()));
		}
	}
	
	else if ((a_->m_bullet_cmp && b_->m_crevasse_cmp) // bullet vs crevasse
		|| (a_->m_crevasse_cmp && b_->m_bullet_cmp)) {
		c_object* bullet = nullptr;
		c_object* crevasse = nullptr;
		if (a_->m_bullet_cmp) {
			bullet = a_;
			crevasse = b_;
		} else {
			bullet = b_;
			crevasse = a_;
		}

		auto bullet_sphere = dynamic_cast<c_sphere*>(bullet->m_collidables[0]);

		// todo optimize with bb
		for (unsigned i = 0; i < crevasse->m_collidables.size(); ++ i) {
			auto wall = crevasse->m_collidables[i];
			auto wall_as_plane = dynamic_cast<c_plane*>(wall);
			if (!c_collision::check_single_face_plane_with_boundaries_vs_sphere(wall_as_plane->m_normal_dir,
																				wall_as_plane->m_normal_up_dir,
																				wall_as_plane->m_origin,
																				wall_as_plane->m_drawing_width,
																				wall_as_plane->m_drawing_height,
																				bullet_sphere,
																				wall_as_plane->m_thickness))
				continue;
			crevasse->m_last_collision_events.push_back(c_collision_event(bullet,
																			Vector3f::Zero()));
			bullet->m_last_collision_events.push_back(c_collision_event(crevasse,
																			Vector3f::Zero()));
		}
	}
	// bullet vs terrain
	else if ((a_->m_bullet_cmp && b_->m_terrain_cmp)
		|| (a_->m_terrain_cmp && b_->m_bullet_cmp)) {
		c_object* bullet = nullptr;
		c_object* terrain = nullptr;
		if (a_->m_bullet_cmp) {
			bullet = a_;
			terrain = b_;
		} else {
			bullet = b_;
			terrain = a_;
		}

		// check bullet pos is in terrain's boundary
		Vector3f bullet_pos = bullet->get_pos() + (bullet->m_bullet_cmp->m_vel * dt_);
		if (((bullet_pos.x() + bullet->m_bullet_cmp->m_hit_sphere_radius) 
				< terrain->m_terrain_cmp->m_boundary_min_x)
			|| ((bullet_pos.z() + bullet->m_bullet_cmp->m_hit_sphere_radius)
				< terrain->m_terrain_cmp->m_boundary_min_z)
			|| ((bullet_pos.x() - bullet->m_bullet_cmp->m_hit_sphere_radius)
				> terrain->m_terrain_cmp->m_boundary_max_x)
			|| ((bullet_pos.z() - bullet->m_bullet_cmp->m_hit_sphere_radius)
				> terrain->m_terrain_cmp->m_boundary_max_z))
			return;
		auto terrain_plane = dynamic_cast<c_plane*>(terrain->m_collidables[0]);
		auto bullet_sphere = dynamic_cast<c_sphere*>(bullet->m_collidables[0]);
		if (!c_collision::check_single_face_plane_with_boundaries_vs_sphere(terrain_plane->m_normal_dir,
																			terrain_plane->m_normal_up_dir,
																			terrain_plane->m_origin,
																			terrain_plane->m_drawing_width,
																			terrain_plane->m_drawing_height,
																			bullet_sphere,
																			terrain_plane->m_thickness))
			return;
		terrain->m_last_collision_events.push_back(c_collision_event(bullet,
																		Vector3f::Zero()));
		bullet->m_last_collision_events.push_back(c_collision_event(terrain,
																		Vector3f::Zero()));
	}
}

void c_in_game::render(float dt_,
					Matrix4f view_mat_,
					Matrix4f proj_mat_) {
	if (m_player) {
		m_player->render(dt_,
						view_mat_,
						proj_mat_);

		if (m_player->m_player_cmp)
			m_player->m_player_cmp->render_gun(dt_, view_mat_, proj_mat_);
	}

	if (m_terrain)
		m_terrain->render(dt_, view_mat_, proj_mat_);

	for (unsigned i = 0; i < m_bullets.size(); ++ i) {
		auto bullet = m_bullets[i];
		if (bullet == nullptr)
			continue;
		bullet->render(dt_,
						view_mat_,
						proj_mat_);
	}

	for (unsigned i = 0; i < m_crevasses.size(); ++ i) {
		auto crevasse = m_crevasses[i];
		if (crevasse == nullptr)
			continue;
		crevasse->render(dt_,
						view_mat_,
						proj_mat_);
	}

	for (unsigned i = 0; i < m_tree_groups.size(); ++ i) {
		auto tree_group = m_tree_groups[i];
		if (tree_group) {
			tree_group->render(dt_, view_mat_, proj_mat_);
		}
	}

	for (unsigned i = 0; i < m_wind_blowers.size(); ++ i) {
		auto wind_blower = m_wind_blowers[i];
		if (wind_blower == nullptr)
			continue;
		wind_blower->render(dt_,
						view_mat_,
						proj_mat_);
	}

	for (unsigned i = 0; i < m_light_emitters.size(); ++ i) {
		auto light_emitter = m_light_emitters[i];
		if (light_emitter == nullptr)
			continue;
		light_emitter->render(dt_,
						view_mat_,
						proj_mat_);
	}

	for (unsigned i = 0; i < m_other_objects.size(); ++ i) {
		auto other_object = m_other_objects[i];
		if (other_object == nullptr)
			continue;

		other_object->render(dt_,
						view_mat_,
						proj_mat_);

	}
}

void c_in_game::debug_render(float dt_,
								Matrix4f view_mat_,
								Matrix4f proj_mat_) {
	//if (m_is_debug_build) {
	//	for (unsigned i = 0; i < m_lights.size(); ++ i) {
	//		auto lt = m_lights[i];
	//		if (lt)
	//			lt->debug_render(dt_, view_mat_, proj_mat_);
	//	}
	//}


	if (m_render_debug) {
		for (unsigned i = 0; i < m_tree_groups.size(); ++ i) {
			auto tree_group = m_tree_groups[i];
			if (tree_group) {
				tree_group->debug_render(dt_, view_mat_, proj_mat_);
			}
		}

		for (unsigned i = 0; i < m_crevasses.size(); ++ i) {
			auto crevasse = m_crevasses[i];
			if (crevasse == nullptr)
				continue;
			crevasse->debug_render(dt_,
							view_mat_,
							proj_mat_);
		}




		for (unsigned i = 0; i < m_bullets.size(); ++ i) {
			auto bullet = m_bullets[i];
			if (bullet == nullptr)
				continue;
			bullet->debug_render(dt_,
							view_mat_,
							proj_mat_);
		}

		if (m_world_oct)
			m_world_oct->debug_render(dt_, view_mat_, proj_mat_);

		//if (m_player) {
		//	//m_player->debug_render(dt_,
		//	//				view_mat_,
		//	//				proj_mat_);

		//	if (m_player->m_player_cmp)
		//		m_player->m_player_cmp->debug_render_gun(dt_, view_mat_, proj_mat_);
		//}

		if (m_terrain)
			m_terrain->debug_render(dt_, view_mat_, proj_mat_);

		for (unsigned i = 0; i < m_wind_blowers.size(); ++ i) {
			auto wind_blower = m_wind_blowers[i];
			if (wind_blower == nullptr)
				continue;
			wind_blower->debug_render(dt_,
							view_mat_,
							proj_mat_);
		}

		for (unsigned i = 0; i < m_light_emitters.size(); ++ i) {
			auto light_emitter = m_light_emitters[i];
			if (light_emitter == nullptr)
				continue;
			light_emitter->debug_render(dt_,
							view_mat_,
							proj_mat_);
		}

		for (unsigned i = 0; i < m_other_objects.size(); ++ i) {
			auto other_object = m_other_objects[i];
			if (other_object == nullptr)
				continue;

			other_object->debug_render(dt_,
							view_mat_,
							proj_mat_);

		}
	}
}

void c_in_game::finish() {
}

void c_in_game::destroy() {
	if (m_camera != nullptr) {
		delete m_camera;
		m_camera = nullptr;
	}

	for (unsigned i = 0; i < m_tree_groups.size(); ++ i) {
		auto tree_group = m_tree_groups[i];
		if (tree_group) {
			tree_group->destroy();
			delete tree_group;
		}
	}
	m_tree_groups.clear();

	c_shader_mgr::get()->m_program_phong->destroy();
	c_shader_mgr::get()->m_program_debug_shader->destroy();

	for (unsigned i = 0; i < m_bullets.size(); ++ i)
		delete m_bullets[i];
	m_bullets.clear();

	for (unsigned i = 0; i < m_crevasses.size(); ++ i)
		delete m_crevasses[i];
	m_crevasses.clear();

	for (unsigned i = 0; i < m_wind_blowers.size(); ++ i)
		delete m_wind_blowers[i];
	m_wind_blowers.clear();

	for (unsigned i = 0; i < m_light_emitters.size(); ++ i)
		delete m_light_emitters[i];
	m_light_emitters.clear();

	for (unsigned i = 0; i < m_other_objects.size(); ++ i)
		delete m_other_objects[i];
	m_other_objects.clear();

	if (m_player) {
		delete m_player;
		m_player = nullptr;
	}

	if (m_terrain) {
		delete m_terrain;
		m_terrain = nullptr;
	}

	if (m_world_oct) {
		delete m_world_oct;
		m_world_oct = nullptr;
	}

	for (unsigned i = 0; i < m_lights.size(); ++ i) {
		auto lt = m_lights[i];
		delete lt;
	}
	m_lights.clear();	
}

void c_in_game::add_directional_light(c_directional_light* directional_lt_) {
	if (m_lights.size() >= m_max_num_of_lights)
		return;
	m_lights.push_back(directional_lt_);
	++ m_phong_prop_current_light_count;
}

void c_in_game::add_point_light(c_point_light* point_lt_) {
	if (m_lights.size() >= m_max_num_of_lights)
		return;
	m_lights.push_back(point_lt_);
	++ m_phong_prop_current_light_count;
}

void c_in_game::add_spot_light(c_spot_light* spot_lt_) {
	if (m_lights.size() >= m_max_num_of_lights)
		return;				
	m_lights.push_back(spot_lt_);
	++ m_phong_prop_current_light_count;
}

bool c_in_game::is_ring_out(Vector3f pos_to_check_) {
	if (pos_to_check_.y() < m_ring_out_y)
		return true;
	float pos_x = pos_to_check_.x();
	float pos_z = pos_to_check_.z();

	if ((pos_x < m_ring_out_min_x)
		|| (pos_z < m_ring_out_min_z)
		|| (pos_x > m_ring_out_max_x)
		|| (pos_z > m_ring_out_max_z))
		return true;
	return false;
}

bool c_in_game::query_raycast(Vector3f start_,
								Vector3f dir_,
								float& out_t_,
								c_object* out_picked_) {
	if (!m_world_oct)
		return false;
	return m_world_oct->query_raycast(start_, dir_, out_t_, out_picked_);
}