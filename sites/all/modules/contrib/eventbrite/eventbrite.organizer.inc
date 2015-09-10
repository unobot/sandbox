<?php

/**
 * @file
 * Defines venue handling functions
 */

function eventbrite_organizer_form($default_values = array()) {
  $form = array();
  
  if (isset($default_values['organizer_id'])) {  
    $form['organizer_id'] = array(
      '#type' => 'value',
      '#value' => $default_values['organizer_id'],
    );
  }
  
  // Name is the only required field
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#default_value' => isset($default_values['name']) ? $default_values['name'] : '',
    '#required' => 1,
    '#maxlength' => 32,
  );

  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description'),
    '#default_value' => isset($default_values['description']) ? $default_values['description'] : '',
  );
  
  return $form;
}

function eventbrite_organizer_load($organizer_id) {
  $result = db_query('SELECT * FROM {eventbrite_organizer} WHERE organizer_id = %d', $organizer_id);
  if ($row = db_fetch_array($result)) {
    return $row;
  }
}

/*
 * Organizer Edit Form
 */
function eventbrite_organizer($form_state, $organizer_id = NULL) {
  $form = array();

  if (isset($organizer_id) && $organizer = eventbrite_organizer_load($organizer_id)) {
    $form = eventbrite_organizer_form($organizer);
  
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Update organizer information'),
    );
  }
  else {
    $form = eventbrite_organizer_form();
  
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create this organizer'),
    );
  }
  
  return $form;
}

/*
 * Default Payment Settings Form Validate
 */
function eventbrite_organizer_submit($form, &$form_state) {
  $organizer = _eventbrite_populate_organizer_from_form($form_state['values']);
  eventbrite_organizer_save($organizer);

  return;
}

/*
 * This function pulls venue from the form results and builds a venue object for base Eventbrite module
 */
function _eventbrite_populate_organizer_from_form($item) {
  $organizer = new stdClass();

  if (!empty($item['organizer_id'])) {
    $organizer->organizer_id = $item['organizer_id'];
  }
  $organizer->name = $item['name'];
  $organizer->description = $item['description'];

  return $organizer;
}

function eventbrite_organizer_get_enabled() {
  // If Eventbrite OG is enabled and there is a group context
  // TODO: This filtering of organizers by sub modules should really happen via a hook
  if (module_exists('eventbrite_og') && $group_node = og_get_group_context()) {
    $filter_organizers = eventbrite_og_get_enabled_organizers($group_node);
  }
  else {
    // If Eventbrite OG is not overriding this field, use default user key for filtering
    $default_organizers = eventbrite_get('user_list_organizers');
    $filter_organizers = array_keys($default_organizers);
  }
  
  $enabled = array();
  $result = db_query('SELECT organizer_id, name FROM {eventbrite_organizer} WHERE status = 1');
  while ($row = db_fetch_object($result)) {
    if (!isset($filter_organizers) || in_array($row->organizer_id, $filter_organizers)) {
      $enabled[$row->organizer_id] = $row->name;
    }
  }
  return $enabled;
}

function eventbrite_organizer_list_form($form_state, $user_key = '') {
  // Set default user key to site wide Eventbrite user key
  if (empty($user_key)) {
    $user_key = variable_get('eventbrite_user_key', '');
    if (empty($user_key)) {
      return t('No Eventbrite user key has been configured for this account.');
    }
  }

  // This refreshes organizers for the site-wide account
  $organizers = eventbrite_get('user_list_organizers');

  if ($organizers) {
    $form = array();

    $header = array(
      array(),
      array('data' => t('Organizer Name'), 'field' => 'eo.name'),
      array('data' => t('URL'), 'field' => 'eo.url'),
      array('data' => t('Description'), 'field' => 'eo.description'),
    );
    
    $sql = 'SELECT eo.organizer_id, eo.status, eo.name, eo.url, eo.description FROM {eventbrite_organizer} eo WHERE user_key = "'. $user_key .'"';
    $query_count = 'SELECT COUNT(eo.organizer_id) FROM {eventbrite_organizer} eo WHERE user_key = "'. $user_key .'"';

    $sql .= tablesort_sql($header);

    // TODO: This should display all results, no need for a pager query
    $result = pager_query($sql, 50, 0, $query_count);

    $destination = drupal_get_destination();

    $organizer_array = array();
    while ($organizer = db_fetch_object($result)) {
      $organizer_id = $organizer->organizer_id;
      $organizer_array[$organizer_id] = '';

      $form['name'][$organizer_id] = array(
        '#value' => l(
          $organizer->name,
          'admin/settings/eventbrite/organizers/'.$organizer_id,
          array('query' => $destination)
        ),
      );
      $form['url'][$organizer_id] = array(
        '#value' => l(
          $organizer->url,
          $organizer->url
        ),
      );
      $form['description'][$organizer_id] = array('#value' => $organizer->description);
    }
    
    $enabled_organizers = eventbrite_organizer_get_enabled();
    $form['enabled_organizers'] = array(
      '#type' => 'checkboxes',
      '#options' => $organizer_array,
      '#default_value' => array_keys($enabled_organizers),
    );

    if ($enabled_organizers) {
      $form['default_organizer'] = array(
        '#type' => 'select',
        '#title' => t('Default Organizer'),
        '#default_value' => variable_get('eventbrite_organizer_default', ''),
        '#options' => $enabled_organizers,
        '#required' => TRUE,
      );
    }
    
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save enabled organizers'),
      '#weight' => 100,
    );
    $form['pager'] = array('#value' => theme('pager', NULL, 50, 0));

    return $form;
  }
  $form = array();
  drupal_set_message(t('There are currently no organizers conifigured for this user account.'));
  return $form;
}

/*
 * Eventbrite Organizer Form Validate
 */
function eventbrite_organizer_list_form_validate($form, &$form_state) {
  // TODO: Check that default organizer is an enabled organizer
  $default_organizer = $form_state['values']['default_organizer'];
  $enabled_organizers = $form_state['values']['enabled_organizers'];

  if (array_sum($enabled_organizers) == 0) {
    form_set_error('enabled_organizers', t('You must enable at least one Organizer.'));
  }

  if (isset($enabled_organizers[$default_organizer])) {
    if (empty($enabled_organizers[$default_organizer])) {
      form_set_error('enabled_organizers', t('Your default Organizer must be enabled in the Organizer list.'));
    }
  }
}

function eventbrite_organizer_list_form_submit($form, &$form_state) {
  $enabled_organizers = $form_state['values']['enabled_organizers'];

  variable_set('eventbrite_organizer_default', $form_state['values']['default_organizer']);
  
  // Get current status for all organizers in DB
  $db_organizers = array();
  // Make sure DB matches enabled organizers list
  $result = db_query('SELECT organizer_id, status FROM {eventbrite_organizer} eo WHERE 1');
  while ($organizer = db_fetch_object($result)) {
    $db_organizers[$organizer->organizer_id] = $organizer;
  }

  // Scroll through organizers returned by form.  Since this was a pager query we don't
  //   want to affect any organizers that weren't listed on this pager page
  foreach ($enabled_organizers as $organizer_id => $form_organizer_status) {
    $form_organizer_status = ($form_organizer_status) ? 1 : 0;
    // For each organizer, if status has changed update the DB
    $db_organizer = $db_organizers[$organizer_id];
    if ($form_organizer_status != $db_organizer->status) {
      $db_organizer->status = $form_organizer_status;
      drupal_write_record('eventbrite_organizer', $db_organizer, array('organizer_id'));
      // TODO: Log to watchdog if this doesn't write
    }
  }

}


function eventbrite_organizer_save($organizer) {
  //  Check to see if this organizer already exists on Eventbrite server by organizer_id
  if (isset($organizer->organizer_id) && !empty($organizer->organizer_id)) {
  	// This venue already has an organizer_id, so organizer_update is the API call
    if ($saved_result = eventbrite_save('organizer_update', $organizer)) {
      drupal_set_message(t('This organizer has been updated on the Eventbrite server.'));
    }
    else {
      drupal_set_message(t('This organizer could not be updated on the Eventbrite server'), 'error');
      return;
    }
  }
  else {
    // This organizer does not exist, so organizer_new is the API call
    if ($saved_result = eventbrite_save('organizer_new', $organizer)) {
      drupal_set_message(t('This organizer has been created on the Eventbrite server.'));
    }
    else {
      drupal_set_message(t('This organizer could not be created on the Eventbrite server'), 'error');
      return;
    }
  }
  if ($saved_result) {
  	eventbrite_organizer_save_local($organizer);
  	return $saved_result;
  }
}

function eventbrite_organizer_save_local($organizer) {
  // Only save locally if there is already an Eventbrite Organizer ID
  if (isset($organizer->organizer_id) && !empty($organizer->organizer_id)) {
  	$result = db_query(
      'SELECT * FROM {eventbrite_organizer} WHERE organizer_id = %d',
      $organizer->organizer_id
    );
    if ($row = db_fetch_object($result)) {
      // This event already exists in the local DB, so update it
      return drupal_write_record('eventbrite_organizer', $organizer, array('organizer_id'));
    }
    else {
      // This venue does not exist in the local DB, so insert it
      // Set status here
      if (empty($organizer->name)) {
	$organizer->status = 0;
      }
      else {
	$organizer->status = 1;
      }
      return drupal_write_record('eventbrite_organizer', $organizer);
    }
  }
}

/* Theme organizer list form.
 *
 * @ingroup themeable
 */
function theme_eventbrite_organizer_list_form($form) {
  $header = array(
    theme('table_select_header_cell'),
    array('data' => t('Organizer Name'), 'field' => 'eo.name'),
    array('data' => t('URL'), 'field' => 'eo.url'),
    array('data' => t('Description'), 'field' => 'eo.description'),
  );
  
  $output = drupal_render($form['default_organizer']);
  
  $output .= drupal_render($form['options']);
  if (isset($form['name']) && is_array($form['name'])) {
    foreach (element_children($form['name']) as $key) {
      $rows[] = array(
        drupal_render($form['enabled_organizers'][$key]),
        drupal_render($form['name'][$key]),
        drupal_render($form['url'][$key]),
        drupal_render($form['description'][$key]),
      );
    }
  }
  else {
    $rows[] = array(array('data' => t('No organizers were found in this Eventbrite account.'), 'colspan' => '7'));
  }

  $output .= '<div class="form-item"><label>'. t('Enabled Organizers:') .'</label></div>';
  $output .= theme('table', $header, $rows);
  if ($form['pager']['#value']) {
    $output .= drupal_render($form['pager']);
  }

  $output .= drupal_render($form);

  return $output;
}
