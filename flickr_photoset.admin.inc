<?php
/**
 * List operation of hook_block().
 */
function _flickr_photoset_block_list($delta, $edit) {
	$blocks = array();
  foreach (variable_get('flickr_photoset_ids', array()) as $delta) {
    $blocks[$delta]['info'] = _flickr_photoset_format_title($delta);
  }
  return $blocks;
}

/**
 * Format a block title for display on blocks page.
 */
function _flickr_photoset_format_title($delta) {
	$photoset = variable_get("flickr_photoset_block_{$delta}_photoset_id", '');
	if(!empty($photoset)) {
		return t('Flickr photoset: @photoset', array('@photoset' => $photoset));
	}
	else {
		return t('Flickr photoset');
	}
}

/**
 * Configure operation of hook_block().
 */
function _flickr_photoset_block_configure($delta, $edit) {
	// Load Flickr module API
	module_load_include('inc', 'flickr', 'flickr');
	
	// Count options
	$count_options = array();
	for($i=1; $i<=30; $i++) {
		$count_options[$i] = $i;
	}
	
	// Size options
	$size_options = array();
  foreach (flickr_photo_sizes() as $size => $info) {
    $size_options[$size] = $info['label'] .' - '. $info['description'];
  }
  
  // Assemble form
  $form = array();
  
  // Number of photos to display
  $form["flickr_photoset_block_show_n"] = array(
    '#type' => 'select',
    '#options' => $count_options,
    '#title' => t('Show <em>n</em> photos'),
    '#default_value' => variable_get("flickr_photoset_block_{$delta}_show_n", 4),
    '#description' => t("The block will display this many photos.")
  );
  
  // The size of the photos
  $form["flickr_photoset_block_size"] = array(
    '#type' => 'select',
    '#options' => $size_options,
    '#title' => t('Size of photos'),
    '#default_value' => variable_get("flickr_photoset_block_{$delta}_size", 's'),
    '#description' => t("Select the size of photos you'd like to display in the block.")
  );
  
  // The photoset ID
  $form["flickr_photoset_block_photoset_id"] = array(
    '#type' => 'textfield',
    '#title' => t('Flickr photoset ID'),
    '#default_value' => variable_get("flickr_photoset_block_{$delta}_photoset_id", ''),
    '#description' => t("Enter the Flickr photoset id, http://www.flickr.com/photos/<tt>user_id</tt>/sets/<tt>set_id</tt>/")
  );
  
  // Return
  return $form;
}

/**
 * Save operation of hook_block().
 */
function _flickr_photoset_block_save($delta, $edit) {	
	variable_set("flickr_photoset_block_{$delta}_show_n", $edit["flickr_photoset_block_show_n"]);
  variable_set("flickr_photoset_block_{$delta}_size", $edit["flickr_photoset_block_size"]);
  variable_set("flickr_photoset_block_{$delta}_photoset_id", $edit["flickr_photoset_block_photoset_id"]);
}

/**
 * Add block form.
 */
function flickr_photoset_add_block_form(&$form_state) {
	include_once './' . drupal_get_path('module', 'block') . '/block.admin.inc';
  return block_admin_configure($form_state, 'flickr_photoset', NULL);
}

/**
 * Handler for block form submit.
 */
function flickr_photoset_add_block_form_submit($form, &$form_state) {
	// Determine the delta of the new block.
  $block_ids = variable_get('flickr_photoset_ids', array());
  $delta = empty($block_ids) ? 1 : max($block_ids) + 1;

  // Save the new array of blocks IDs.
  $block_ids[] = $delta;
  variable_set('flickr_photoset_ids', $block_ids);

  // Save the block configuration.
  _flickr_photoset_block_save($delta, $form_state['values']);

  // Run the normal new block submission (borrowed from block_add_block_form_submit).
  foreach (list_themes() as $key => $theme) {
    if ($theme->status) {
      db_query("INSERT INTO {blocks} (visibility, pages, custom, title, module, theme, status, weight, delta, cache) VALUES(%d, '%s', %d, '%s', '%s', '%s', %d, %d, %d, %d)", $form_state['values']['visibility'], trim($form_state['values']['pages']), $form_state['values']['custom'], $form_state['values']['title'], $form_state['values']['module'], $theme->name, 0, 0, $delta, BLOCK_NO_CACHE);
    }
  }

  foreach (array_filter($form_state['values']['roles']) as $rid) {
    db_query("INSERT INTO {blocks_roles} (rid, module, delta) VALUES (%d, '%s', '%s')", $rid, $form_state['values']['module'], $delta);
  }

  drupal_set_message(t('The block has been created.'));
  cache_clear_all();

  $form_state['redirect'] = 'admin/build/block';
  return;
}

/**
 * Alters the block admin form to add delete links next to menu blocks.
 */
function _flickr_photoset_form_block_admin_display_form_alter(&$form, $form_state) {
  foreach (variable_get('flickr_photoset_ids', array()) as $delta) {
    $form['flickr_photoset_' . $delta]['delete'] = array('#value' => l(t('delete'), 'admin/build/block/delete-flickr-photoset/'. $delta));
  }
}

/**
 * Confirm delete block.
 */
function flickr_photoset_delete(&$form_state, $delta = 0) {
  $title = _flickr_photoset_format_title($delta);
  $form['block_title'] = array('#type' => 'hidden', '#value' => $title);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the "%name" block?', array('%name' => $title)), 'admin/build/block', NULL, t('Delete'), t('Cancel'));
}

/**
 * Delete block.
 */
function flickr_photoset_delete_submit($form, &$form_state) {
	// Remove the menu block configuration variables.
  $delta = $form_state['values']['delta'];
  $block_ids = variable_get('flickr_photoset_ids', array());
  unset($block_ids[array_search($delta, $block_ids)]);
  sort($block_ids);
  variable_set('flickr_photoset_ids', $block_ids);
  variable_del("flickr_photoset_{$delta}_show_n");
  variable_del("flickr_photoset_{$delta}_size");
  variable_del("flickr_photoset_{$delta}_photoset_id");

  db_query("DELETE FROM {blocks} WHERE module = 'flickr_photoset' AND delta = %d", $delta);
  db_query("DELETE FROM {blocks_roles} WHERE module = 'flickr_photoset' AND delta = %d", $delta);
  drupal_set_message(t('The "%name" block has been removed.', array('%name' => $form_state['values']['block_title'])));
  cache_clear_all();
  $form_state['redirect'] = 'admin/build/block';
  return;
}
?>