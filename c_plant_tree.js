/*
	tree_node contains position
*/

class c_plant_tree {
	/* private */
	m_root_edge = null;
	m_edges = [];
	m_model_mat;

	m_current_n;
	m_current_l_str;
	m_current_segment_width;

	m_setting_root_pos_x;
	m_setting_root_pos_y;
	m_setting_root_pos_z;
	m_setting_root_orientation_deg_x;
	m_setting_root_orientation_deg_y;
	m_setting_root_orientation_deg_z;

	m_setting_tropism_vec_x;
	m_setting_tropism_vec_y;
	m_setting_tropism_vec_z;
	m_setting_bending_susceptibility;

	m_setting_n;
	m_setting_angle_increment;
	m_setting_step_amount;
	m_setting_trunk_radius;
	m_setting_segment_slice_count;
	m_setting_segment_stack_count;
	m_setting_debug_mode_enabled;

	m_constant_dic = new Map();
	// A(w, l) -> A, ["w", "l"]
	m_func_param_token_map = new Map();

	static gen(l_code_str_) {
		// c_plant_tree.test_parse_param();

		/*
			ω: !(1)F(200)/(45)A
			A: * → !(vr)F(50)[&(a)F(50)A]/(d1)[&(a)F(50)A]/(d2)[&(a)F(50)A]
			F(l): * → F(l*lr)
			!(w): * → !(w*vr)

			!(1) F(200) /(45) A
			!(1) F(200 * lr) /(45) !(vr)F(50)[&(a)F(50)A]/(d1)[&(a)F(50)A]/(d2)[&(a)F(50)A]

		*/

		let code_lines = l_code_str_.split(';');
		if (code_lines.length < 1)
			return;

		let ret = new c_plant_tree();
		ret.m_constant_dic = new Map();
		ret.m_func_param_token_map = new Map();
		ret.m_setting_rule_generator_funcs = new Map();

		// init default
		ret.m_setting_tropism_vec_x = 0;
		ret.m_setting_tropism_vec_y = 0;
		ret.m_setting_tropism_vec_z = 0;
		ret.m_setting_bending_susceptibility = 0;

		for (let i = 0; i < code_lines.length; ++ i) {
			let code_line = code_lines[i].trim();
			if (code_line.length === 0) continue;
			let words = code_line.split(':');
			let lhs = words[0].trim();
			if (lhs[0] == '#') // comment
				continue;
			let rhs = words[1].trim();
			if (lhs === 'n')
				ret.m_setting_n = parseInt(rhs);
			else if (lhs === "angle increment")
				ret.m_setting_angle_increment = parseFloat(rhs);
			else if (lhs === "step amount")
				ret.m_setting_step_amount = parseFloat(rhs);
			else if (lhs === "trunk radius")
				ret.m_setting_trunk_radius = parseFloat(rhs);
			else if (lhs === "segment slice count")
				ret.m_setting_segment_slice_count = parseInt(rhs);
			else if (lhs === "segment stack count")
				ret.m_setting_segment_stack_count = parseInt(rhs);
			else if (lhs === "root pos x")
				ret.m_setting_root_pos_x = parseFloat(rhs);
			else if (lhs === "root pos y")
				ret.m_setting_root_pos_y = parseFloat(rhs);
			else if (lhs === "root pos z")
				ret.m_setting_root_pos_z = parseFloat(rhs);
			else if (lhs === "root orientation deg x")
				ret.m_setting_root_orientation_deg_x = parseFloat(rhs);
			else if (lhs === "root orientation deg y")
				ret.m_setting_root_orientation_deg_y = parseFloat(rhs);
			else if (lhs === "root orientation deg z")
				ret.m_setting_root_orientation_deg_z = parseFloat(rhs);
			else if (lhs === "tropism vector x")
				ret.m_setting_tropism_vec_x = parseFloat(rhs);
			else if (lhs === "tropism vector y")
				ret.m_setting_tropism_vec_y = parseFloat(rhs);
			else if (lhs === "tropism vector z")
				ret.m_setting_tropism_vec_z = parseFloat(rhs);
			else if (lhs === "bending susceptibility")
				ret.m_setting_bending_susceptibility = parseFloat(rhs);
			else if (lhs === "debug mode enabled") {
				if (rhs === "true")
					ret.m_setting_debug_mode_enabled = true;
				else
					ret.m_setting_debug_mode_enabled = false;
			} else if (lhs[0] === '$') { // constants
				let name_of_constant = lhs.substr(1, lhs.length);
				let value = Number(rhs);
				ret.m_constant_dic.set(name_of_constant, value);
				console.log("constant: " + name_of_constant + ", value: " + value);
			} else if (lhs[0] === 'w') {
				ret.m_setting_rule_initiator_func = function() {
					return rhs;
				}
			} else if (rhs !== "") {
				// gen function parameter map
				// lhs_param_str = "l,w"
				let lhs_params = c_plant_tree.extract_function_params(lhs, 1);
				let replace_function = null;
				if (lhs_params === null) {
					replace_function = function(constant_dic_) {					
						let derived_str = c_plant_tree.derive_rule_string(rhs,
																			constant_dic_);
						return derived_str;
					}			
				} else { // if lhs is function type
					// lhs_params = "l", "w"
					// "!" -> "w"
					ret.m_func_param_token_map.set(lhs[0], lhs_params);

					// sample example for comment
					// A(1)
					// A(w): !(w)F[-A(w * 0.5)][+A(w * r_ratio)];
					// !(1)F[-A(0.5)][+A(0.2)]

					let param_count = lhs_params.length;

					if (param_count === 1) {
						replace_function = function(constant_dic_,
													param_list_,
													a_) {					
							let derived_str = c_plant_tree.derive_rule_string(rhs,
																				constant_dic_,
																				param_list_,
																				a_);
							return derived_str;
						}
					} else if (param_count === 2) {
						replace_function = function(constant_dic_,
													param_list_,
													a_,
													b_) {
							let derived_str = c_plant_tree.derive_rule_string(rhs,
																				constant_dic_,
																				param_list_,
																				a_,
																				b_);
							return derived_str;
						}
					} else if (param_count === 3) {
						replace_function = function(constant_dic_,
													param_list_,
													a_,
													b_,
													c_) {
							let derived_str = c_plant_tree.derive_rule_string(rhs,
																				constant_dic_,
																				param_list_,
																				a_,
																				b_,
																				c_);
							return derived_str;
						}
					} else if (param_count === 4) {
						replace_function = function(constant_dic_,
													param_list_,
													a_,
													b_,
													c_,
													d_) {
							let derived_str = c_plant_tree.derive_rule_string(rhs,
																				constant_dic_,
																				param_list_,
																				a_,
																				b_,
																				c_,
																				d_);
							return derived_str;
						}
					} else
						console.error("don't support more than 5 paramters!");
				}
				if (replace_function !== null)
					ret.m_setting_rule_generator_funcs.set(lhs[0], replace_function);				
			}
		}

		ret.m_current_n = 0;
		ret.m_current_segment_width = ret.m_setting_trunk_radius * 2.0;
		ret.start();
		return ret;
	}

	static test_parse_param() {
		/*
			A(1,10)
			A(l,w)
			!(w)
			F(l)
			&(a1)
			+(a1)
			/(180)
			&(a2)
			B(l*r2, w*wr)
			$B(l*r1, w*wr)

			C((l * r1 + 1) + 1)
		*/
		let result1 = c_plant_tree.parse_param("3 + 4 * 2 / ( 1 - 5 ) * 2 / 3");
		console.log("result1: " + result1);

		let result2 = c_plant_tree.parse_param("( ( 3 + 4 ) * ( 2 + 3 ) )");
		console.log("result2: " + result2);

		let result3 = c_plant_tree.parse_param("( ( 31 + 42 ) * ( 52 - 132 ) )");
		console.log("result3: " + result3);

		let constant_dic = new Map();
		constant_dic.set('k', 31);
		constant_dic.set('kb', 42);
		let result4 = c_plant_tree.parse_param("( ( k + kb ) * ( 52 - 132 ) )", constant_dic);
		console.log("result4: " + result4);
	}

	static is_token_operator(token_) {
		if (token_ === '+'
			|| token_ === '-'
			|| token_ === '*'
			|| token_ === '/')
			return true;
		return false;
	}

	static derive_rule_string(rule_str_,
								constant_dic_,
								param_list_ = null,
								a_ = null,
								b_ = null,
								c_ = null,
								d_ = null) {
		// sample example for comment
		// rule_str_ = "!(w)F((l))[+(a1)$B(l * r1, ((w) * wr))][-(a2)$B(l * r2, w * wr)]"
		// ret.m_func_param_token_map.set(lhs[0], lhs_params);
		// A, ["w",
		let func_param_dic = new Map();
		if (param_list_ !== null) {
			for (let i = 0; i < param_list_.length; ++ i) {
				let func_param_token = param_list_[i].trim();
				if (i === 0)
					func_param_dic.set(func_param_token, a_.trim());
				else if (i === 1)
					func_param_dic.set(func_param_token, b_.trim());
				else if (i === 2)
					func_param_dic.set(func_param_token, c_.trim());
				else if (i === 3)
					func_param_dic.set(func_param_token, d_.trim());
			}
		}

		let str_to_ret = "";
		let open_count = 0;
		let closed_count = 0;
		let function_start_pos = null;
		let function_end_pos = null;
		for (let i = 0; i < rule_str_.length; ++ i) {
			let token = rule_str_[i];

			if (token === '(') {
				++ open_count;
				if (function_start_pos === null)
					function_start_pos = i;
			} else if (token == ')') {
				++ closed_count;
				if (open_count > 0
					&& open_count === closed_count) {
					function_end_pos = i;
		
					if (function_start_pos !== null && function_end_pos !== null) {
						let substr_length = function_end_pos - (function_start_pos + 1);
						let within_str = rule_str_.substr(function_start_pos + 1, substr_length);
						within_str = within_str.trim();
						let func_params = within_str.split(',');
						str_to_ret += '(';
						for (let j = 0; j < func_params.length; ++ j) {
							let func_param = func_params[j].trim();
							let result = c_plant_tree.parse_param(func_param,
																	constant_dic_,
																	func_param_dic);
							str_to_ret += result.toString();
							if ((j + 1) < func_params.length)
								str_to_ret += ", ";
						}
						str_to_ret += ')';
						i = function_end_pos;
						function_start_pos = null;
						function_end_pos = null;
						open_count = 0;
						closed_count = 0;
						continue;
					}
				}				
			} else {
				if (open_count === 0) {
					str_to_ret += token;
				}
			}
		}
		return str_to_ret;
	}

	static find_pos_within_parenthesis(str_, start_pos_) {
		// sample
		// str_ = "(a_, b_)" -> return = "a_", "b_" 
		// str_ = (a_, b_)F() -> return = "a_", "b_"
		// str_ = ((a_ + c_), b_) -> return = "(a_ + c_)", "b_"

		// cut the 
		let open_count = 0;
		let closed_count = 0;
		let function_start_pos = null;
		let function_end_pos = null;
		for (let i = start_pos_; i < str_.length; ++ i) {
			let current = str_[i];
			if (current === '(') {
				++ open_count;
				if (function_start_pos === null)
					function_start_pos = i;
			} else if (current == ')')
				++ closed_count;
			
			if (open_count > 0
				&& open_count === closed_count) {
				function_end_pos = i;
				break;
			}
		}

		if (function_start_pos !== null && function_end_pos !== null)
			return [function_start_pos, function_end_pos];
		return null;
	}	

	static extract_function_params(str_, start_pos_) {
		let str_pos_pair = c_plant_tree.find_pos_within_parenthesis(str_, start_pos_);		
		if (str_pos_pair !== null) {
			let function_start_pos = str_pos_pair[0];
			let function_end_pos = str_pos_pair[1];
			let substr_length = function_end_pos - (function_start_pos + 1);
			let within_str = str_.substr(function_start_pos + 1, substr_length);
			within_str = within_str.trim();
			let params = within_str.split(',');
			return params;
		}
		return null;
	}

	// https://en.wikipedia.org/wiki/Shunting-yard_algorithm
	static parse_param(str_,
						constant_dic_,
						func_param_dic_) {		
		// 3 + 4 * 2 / ( 1 - 5 ) * 2 / 3
		// end of first while loop
		// 	operator stack: +, /
		// 	output queue: 3, 4, 2, *, 1, 5, -, /, 2, *, 3
		// end of second while loop
		// 	operator stack:
		// 	output queue: 3, 4, 2, *, 1, 5, -, /, 2, *, 3, /, +

		// ( ( 3 + 4 ) * ( 2 + 3 ) )
		// end of first while loop
		// 	operator stack: 
		// 	output queue: 3, 4, +, 2, 3, +, *
		// end of second while loop
		// 	operator stack:
		// 	output queue: 3, 4, +, 2, 3, +, *

		let tokens = str_.split(' ');

		let operator_stack = [];
		let output_queue = [];

		let precedence_table = new Map();
		precedence_table.set('+', 2);
		precedence_table.set('-', 2);
		precedence_table.set('*', 3);
		precedence_table.set('/', 3);

		for (let i = 0; i < tokens.length; ++ i) {
			let current = tokens[i];
			if (current === ' ')
				continue;
			else if (current === '(') {
				operator_stack.push(current);
			} else if (current === ')') {
				let rp_loop_cond = true;
				while (rp_loop_cond) {
					if (operator_stack.length > 0) {
						if (operator_stack[operator_stack.length - 1] !== '(') {
							let token = operator_stack.pop();
							output_queue.push(token);
						} else
							rp_loop_cond = false;
					} else
						rp_loop_cond = false;
				}
				operator_stack.pop();
			} else if (c_plant_tree.is_token_operator(current) === true) {
				let o1_precedence = precedence_table.get(current);
				let o2_loop_cond = true;
				while (o2_loop_cond === true) {
					if (operator_stack.length > 0) {
						let o2 = operator_stack[operator_stack.length - 1];
						if (c_plant_tree.is_token_operator(o2) === true) {
							let o2_precedence = precedence_table.get(o2);
							if (o2_precedence >= o1_precedence) {
								// pop o2 from the operator stack into the output queue
								o2 = operator_stack.pop();
								output_queue.push(o2);
							} else
								o2_loop_cond = false;
						} else
							o2_loop_cond = false;
					} else
						o2_loop_cond = false;
				}
				// push o1 onto the operator stack
				operator_stack.push(current);
			} else if (constant_dic_ !== undefined && constant_dic_.has(current) === true) {
				let num_str = constant_dic_.get(current);
				let num = Number(num_str);
				output_queue.push(num);
			} else if (func_param_dic_ !== undefined && func_param_dic_.has(current) === true) {
				let num_str = func_param_dic_.get(current);
				let num = Number(num_str);
				output_queue.push(num);
			} else { // just number
				let num = Number(current);
				output_queue.push(num);
			}
		}

		// After the while loop, pop the remaining items from the operator stack into the output queue.
		let loop_cond = true;
		while (loop_cond) {
			if (operator_stack.length > 0) {
				let token = operator_stack.pop();
				output_queue.push(token);
			} else
				loop_cond = false;
		}

		let operator_func_map = new Map();
		operator_func_map.set('+',
			function(lhs_, rhs_) {
				return lhs_ + rhs_;
			});
		operator_func_map.set('-',
			function(lhs_, rhs_) {
				return lhs_ - rhs_;
			});
		operator_func_map.set('*',
			function(lhs_, rhs_) {
				return lhs_ * rhs_;
			});
		operator_func_map.set('/',
			function(lhs_, rhs_) {
				return lhs_ / rhs_;
			});

		// interpret
		for (let i = 0; i < output_queue.length; ++ i) {
			let token = output_queue[i];
			if (c_plant_tree.is_token_operator(token) === true) {
				let prev_token = output_queue[i - 1];
				let prev_prev_token = output_queue[i - 2];
				let result = operator_func_map.get(token)(prev_prev_token, prev_token);
				output_queue[i] = result;
				output_queue.splice(i - 2, 2);
				i = i - 2;
			}
		}

		let result = output_queue[0];
		return result;
	}

	start() {
		if (this.m_setting_debug_mode_enabled === true) {
			this.derive_next();
		} else {
			while (this.m_current_n <= this.m_setting_n) {
				this.derive_next();
			}
		}
	}

	bulid_model_from_str(str_to_gen_, constant_dic_) {
		if (str_to_gen_.length < 1)
			return;

		let setting_turtle_angle_inc_rad = c_math.deg_to_rad(this.m_setting_angle_increment);

		let stack_last_nodes = [];
		let last_node = null;

		let root_orientation_mat = c_mat4.get_rotated(c_math.deg_to_rad(this.m_setting_root_orientation_deg_x),
														c_math.deg_to_rad(this.m_setting_root_orientation_deg_y),
														c_math.deg_to_rad(this.m_setting_root_orientation_deg_z));
		let root_pos = c_vec3.gen(this.m_setting_root_pos_x,
									this.m_setting_root_pos_y,
									this.m_setting_root_pos_z);
		let root_tr_mat = c_mat4.get_translated(root_pos);
		this.m_model_mat = root_tr_mat;
		this.m_model_mat = this.m_model_mat.get_mat4_multiplied(root_orientation_mat);
		// last turtle heading orientation
		// heading, left, up
		
		//										H  L  U
		let last_orientation_mat = c_mat3.gen(0, -1, 0,
												0, 0, 1,
												-1, 0, 0);
							
		//*/
		//last_orientation_mat = last_orientation_mat.get_mat3_multiplied(root_orientation_mat);
		let stack_last_orientation_mats = [];
		let stack_last_segment_widths = [];
		for (let i = 0; i < str_to_gen_.length; ++ i) {
			let delimiter = str_to_gen_[i];
			if (delimiter === '[') {
				// ignore start with branching
				stack_last_nodes.push(last_node);
				stack_last_orientation_mats.push(last_orientation_mat);
				//stack_last_segment_widths.push(this.m_current_segment_width);
			} else if (delimiter === ']') {
				last_node = stack_last_nodes.pop();
				last_orientation_mat = stack_last_orientation_mats.pop();
				//this.m_current_segment_width = stack_last_segment_widths.pop();
			} else if (delimiter === 'F') {
				if (last_node === null)
					last_node = c_plant_tree_node.gen(c_vec3.zero());

				let turtle_pos = last_node.m_pos.get_copied();
				let segment_length = this.m_setting_step_amount;
				let segment_start_width = this.m_current_segment_width;
				let segment_end_width = this.m_current_segment_width;
				
				if ((i + 1) < str_to_gen_.length
					&& str_to_gen_[i + 1] === '(') {
					let str_pos_pair =  c_plant_tree.find_pos_within_parenthesis(str_to_gen_, i + 1);
					if (str_pos_pair !== null) {
						let function_start_pos = str_pos_pair[0];
						let function_end_pos = str_pos_pair[1];
						let substr_length = function_end_pos - (function_start_pos + 1);
						let within_str = str_to_gen_.substr(function_start_pos + 1, substr_length);
						within_str = within_str.trim();
						let params = within_str.split(',');

						for (let j = 0; j < params.length; ++ j) {
							let param = params[j];
							let evaluated = Number(param);
							//let evaluated = parse_param(param, constant_dic_);
							// F(l, w)
							if (j === 0) {
								segment_length = evaluated;
							} else if (j === 1) {
								segment_start_width = evaluated;
								segment_end_width = segment_start_width;
							}
						}
						i = function_end_pos;
					}
				}

				let vec_to_move = last_orientation_mat.get_vec3_multiplied(c_vec3.gen(segment_length,
																			0.0,
																			0.0));
				let next_pos_x = turtle_pos.m_x + vec_to_move.m_x;
				let next_pos_y = turtle_pos.m_y + vec_to_move.m_y;
				let next_pos_z = turtle_pos.m_z + vec_to_move.m_z;

				let ending_pos = c_vec3.gen(next_pos_x, next_pos_y, next_pos_z);

				let enable_bending = false;
				let unbended_end = ending_pos.get_copied();

				let tropism_vec = c_vec3.gen(this.m_setting_tropism_vec_x,
												this.m_setting_tropism_vec_y,
												this.m_setting_tropism_vec_z);
				let do_tropism = !c_math.is_zero(tropism_vec.get_length());
				if (do_tropism === true) {
					enable_bending = false;
					let h_vec = ending_pos.get_vec3_subtracted(turtle_pos);
					let torque = c_math.cross_product(tropism_vec, h_vec);
					let alpha = this.m_setting_bending_susceptibility * torque.get_length();

					//last_turtle_heading_rad = last_turtle_heading_rad + alpha;

					let h_dir = h_vec.get_normalized();
					let tropism_dir = tropism_vec.get_normalized();
					let angle_axis = torque.get_normalized();
					let tropism_rot_mat;
					/*
					if (c_math.is_opposite_direction(cylinder_heading_dir,
														orientation_heading_vec) == true)
						cylinder_rot_mat = c_mat3.get_rotated_x(c_math.deg_to_rad(180.0));
					// flip

					else
					cylinder_rot_mat = c_math.get_rotation_from_vec_to_vec(cylinder_heading_dir,
																	orientation_heading_vec);
												
												
					// get troipsm_dir
					// apply alpha
					get_rotation_from_vec_to_vec	
					
					
					let tropism_rot_mat = c_math.get_rotation_from_vec_to_vec(h_vec.gen_normalized(),
																				tropism_vec.get_normalized());

					let rot_mat = c_mat3.get_rotated_z(parsed_angle);
					let tropism_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);

					let vec_to_move = last_orientation_mat.get_vec3_multiplied(c_vec3.gen(segment_length,
																							0.0,
																							0.0));

					pos_x = turtle_pos.m_x + this.m_setting_step_amount * Math.cos(last_turtle_heading_rad);
					pos_y = turtle_pos.m_y + this.m_setting_step_amount * Math.sin(last_turtle_heading_rad);
					//last_orientation_mat = c_mat4.get_rotated_z(last_turtle_heading_rad);
					*/
					ending_pos = c_vec3.gen(pos_x, pos_y, turtle_pos.m_z);
				}
				let ending_node = c_plant_tree_node.gen(ending_pos);
				this.add_edge(segment_start_width * 0.5,
								segment_end_width * 0.5,
								last_node,
								ending_node,
								last_orientation_mat,
								enable_bending,
								unbended_end);
				last_node = ending_node;
			} else if (delimiter === '+'
						|| delimiter === '-'
						|| delimiter === '&'
						|| delimiter === '^'
						|| delimiter === "\\"
						|| delimiter === '/') {
				let parsed_angle = setting_turtle_angle_inc_rad;
				if ((i + 1) < str_to_gen_.length
					&& str_to_gen_[i + 1] === '(') {
					let str_pos_pair =  c_plant_tree.find_pos_within_parenthesis(str_to_gen_, i + 1);
					if (str_pos_pair !== null) {
						let function_start_pos = str_pos_pair[0];
						let function_end_pos = str_pos_pair[1];
						let substr_length = function_end_pos - (function_start_pos + 1);
						let within_str = str_to_gen_.substr(function_start_pos + 1, substr_length);
						within_str = within_str.trim();
						let params = within_str.split(',');
						let evaluated = Number(params[0]);
						parsed_angle = c_math.deg_to_rad(evaluated);
						i = function_end_pos;
					}
				}
				if (delimiter === '+') {
					let rot_mat = c_mat3.get_rotated_z(parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				} else if (delimiter === '-') {
					let rot_mat = c_mat3.get_rotated_z(-parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				} else if (delimiter === '&') {
					let rot_mat = c_mat3.get_rotated_y(parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				} else if (delimiter === '^') {
					let rot_mat = c_mat3.get_rotated_y(-parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				} else if (delimiter === "\\") {
					let rot_mat = c_mat3.get_rotated_x(parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				} else if (delimiter === '/') {
					let rot_mat = c_mat3.get_rotated_x(-parsed_angle);
					last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
				}
			} else if (delimiter === '!') {  // line width
				if ((i + 1) < str_to_gen_.length
					&& str_to_gen_[i + 1] === '(') {
					let str_pos_pair =  c_plant_tree.find_pos_within_parenthesis(str_to_gen_, i + 1);
					if (str_pos_pair !== null) {
						let function_start_pos = str_pos_pair[0];
						let function_end_pos = str_pos_pair[1];
						let substr_length = function_end_pos - (function_start_pos + 1);
						let within_str = str_to_gen_.substr(function_start_pos + 1, substr_length);
						within_str = within_str.trim();
						let params = within_str.split(',');
						let evaluated = Number(params[0]);
						this.m_current_segment_width = evaluated;
						i = function_end_pos;
					}
				}
			} else if (delimiter === '|') {
				let rot_mat = c_mat3.get_rotated_z(c_math.deg_to_rad(180.0));
				last_orientation_mat = last_orientation_mat.get_mat3_multiplied(rot_mat);
			} else if (delimiter === '$') {
				// direction oppose to gravity
				let v_vec = c_vec3.gen(0.0, 1.0, 0.0);
				let h_vec = c_vec3.gen(last_orientation_mat.m_e00,
										last_orientation_mat.m_e10,
										last_orientation_mat.m_e20);
				let cp_v_h = c_math.cross_product(v_vec, h_vec);
				let new_l = cp_v_h.get_normalized();
				let new_u = c_math.cross_product(h_vec, new_l);
				last_orientation_mat.set(h_vec.m_x, new_l.m_x, new_u.m_x,
											h_vec.m_y, new_l.m_y, new_u.m_y,
											h_vec.m_z, new_l.m_z, new_u.m_z);
			} else if (delimiter === '[') {
				// ignore start with branching
				stack_last_nodes.push(last_node);
				stack_last_orientation_mats.push(last_orientation_mat);
			} else if (delimiter === ']') {
				last_node = stack_last_nodes.pop();
				last_orientation_mat = stack_last_orientation_mats.pop();
			}
		}
	}

	add_edge(segment_start_radius_,
				segment_end_radius_,
				starting_node_,
				opt_ending_node_,
				orientation_mat_,
				enable_bending_,
				unbended_end_) {
		if (opt_ending_node_ === undefined) {
			return null;
		} else {
			let edge_to_add = c_plant_tree_edge.gen(segment_start_radius_,
													segment_end_radius_,
													starting_node_,
													opt_ending_node_,
													orientation_mat_,
													enable_bending_,
													unbended_end_,
													this.m_setting_segment_slice_count,
													this.m_setting_segment_stack_count);
			this.m_edges.push(edge_to_add);
			if (this.m_root_edge === null)
				this.m_root_edge = edge_to_add;
			return edge_to_add;
		}
	}
	
	clear_current() {
		this.m_root_edge = null;
		this.m_edges = [];
	}

	derive_next() {
		if (this.m_current_n === 0)
			this.m_current_l_str = this.m_setting_rule_initiator_func();

		// A(1)
		// A(w): !(w)F[-A(w * 0.5)][+A(w * r_ratio)];
		// !(1)F[-A(0.5)][+A(0.2)] <- this part


		// A(1)

		// "1.0"

		// A, 1.0
		let replaced_str = "";
		for (let i = 0; i < this.m_current_l_str.length; ++ i) {
			let token = this.m_current_l_str[i];
			if (this.m_setting_rule_generator_funcs.has(token) === false)
				replaced_str += token;
			else {
				if ((i + 1) < this.m_current_l_str.length
					&& this.m_current_l_str[i + 1] === '(') { // if delim is param type
					let open_count = 0;
					let closed_count = 0;
					let function_start_pos = null;
					let function_end_pos = null;
					for (let j = i + 1; j < this.m_current_l_str.length; ++ j) {
						let current = this.m_current_l_str[j];
						if (current === '(') {
							++ open_count;
							if (function_start_pos === null)
								function_start_pos = j;
						} else if (current == ')')
							++ closed_count; 
						
						if (open_count > 0
							&& open_count === closed_count) {
							function_end_pos = j;
							break;
						}
					}
					if (function_start_pos !== null && function_end_pos !== null) {
						let substr_length = function_end_pos - (function_start_pos + 1);
						let within_str = this.m_current_l_str.substr(function_start_pos + 1, substr_length);
						within_str = within_str.trim();
						let params = within_str.split(',');
						let param_count = params.length;
						let param_list = this.m_func_param_token_map.get(token);
						if (param_count === 1) {
							let to_replace = this.m_setting_rule_generator_funcs.get(token)(this.m_constant_dic,
																							param_list,
																							params[0]);
							replaced_str += to_replace;
							i = function_end_pos;
						} else if (param_count === 2) {
							let to_replace = this.m_setting_rule_generator_funcs.get(token)(this.m_constant_dic,
																							param_list,
																							params[0],
																							params[1]);
							replaced_str += to_replace;
							i = function_end_pos;
						} else if (param_count === 3) {
							let to_replace = this.m_setting_rule_generator_funcs.get(token)(this.m_constant_dic,
																							param_list,
																							params[0],
																							params[1],
																							params[2]);
							replaced_str += to_replace;
							i = function_end_pos;
						} else if (param_count === 4) {
							let to_replace = this.m_setting_rule_generator_funcs.get(token)(this.m_constant_dic,
																							param_list,
																							params[0],
																							params[1],
																							params[2],
																							params[3]);
							replaced_str += to_replace;
							i = function_end_pos;					
						}
					}
				} else { // if delimiter has no param
					let to_replace = this.m_setting_rule_generator_funcs.get(token)(this.m_constant_dic);
					replaced_str += to_replace;
				}
			}
		}
		this.m_current_l_str = replaced_str;

		if (this.m_setting_debug_mode_enabled === true) {
			this.clear_current();
			this.bulid_model_from_str(this.m_current_l_str);
			console.log("current n: " + this.m_current_n);
			console.log("current l str: " + this.m_current_l_str);
			++ this.m_current_n;
		} else {
			console.log("current n: " + this.m_current_n);
			if (this.m_current_n >= this.m_setting_n)
				this.bulid_model_from_str(this.m_current_l_str);
			++ this.m_current_n;
		}
	}

	render(dt_,
			view_mat_,
			proj_mat_) {
		if (this.m_root_edge !== null) {
			for (let i = 0; i < this.m_edges.length; ++ i) {
				this.m_edges[i].render(dt_,
										this.m_model_mat,
										view_mat_,
										proj_mat_);
			}
		}
	}
}