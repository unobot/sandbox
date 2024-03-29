<?php

/**
 * @file
 * Defines venue handling functions
 */

function eventbrite_venue_form($default_values = array()) {
  $form = array();
  
  if (isset($default_values['venue_id'])) { 
    $form['venue_id'] = array(
      '#type' => 'value',
      '#value' => $default_values['venue_id'],
    );
  }
  
  // Name is the only required field
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Venue Name'),
    '#default_value' => isset($default_values['name']) ? $default_values['name'] : '',
    '#required' => 1,
    '#weight' => 0,
  );

  // TODO: Add Organizer.  Select box if there are more than one, otherwise set the default as a value
  
  $form['address'] = array(
    '#type' => 'textfield',
    '#title' => t('Address Line 1'),
    '#default_value' => isset($default_values['address']) ? $default_values['address'] : '',
    '#weight' => 1,
  );

  $form['address2'] = array(
    '#type' => 'textfield',
    '#title' => t('Address Line 2'),
    '#default_value' => isset($default_values['address2']) ? $default_values['address2'] : '',
    '#weight' => 2,
  );

  $form['city'] = array(
    '#type' => 'textfield',
    '#title' => t('City'),
    '#default_value' => isset($default_values['city']) ? $default_values['city'] : '',
    '#weight' => 3,
  );

  $form['region'] = array(
    '#type' => 'textfield',
    '#title' => t('Region'),
    '#default_value' => isset($default_values['region']) ? $default_values['region'] : '',
    '#weight' => 4,
  );

  $form['postal_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Postal Code'),
    '#default_value' => isset($default_values['postal_code']) ? $default_values['postal_code'] : '',
    '#weight' => 5,
  );

  $countries_api = countries_api_get_array('iso2', 'printable_name');
  asort($countries_api);

  $form['country'] = array(
    '#type' => 'select',
    '#title' => t('Country'),
    '#options' => $countries_api,
    '#default_value' => isset($default_values['country']) ? $default_values['country'] : '',
    '#weight' => 6,
  );


  return $form;
}

// Temporary function
// TODO $form_errors wil have to turn into form_set_errors()
function eventbrite_venue_form_validate($values) {
  $form_errors = array();

  // TODO: Check that there isn't already a venue with this name for this user key
  /*
  if ($values['accept_paypal']) {
  	// If using PayPay check that paypal email is entered
    if (!valid_email_address($values['paypal_email'])) {
      // TODO: Does this need to actually verified that there is a PayPal account registered to this user
  	  $form_errors['paypal_email'] = t('Please provide a valid Paypal email.');
    }
  }
  */
  
  return $form_errors;
}

function eventbrite_venue_load($venue_id) {
  $result = db_query('SELECT * FROM {eventbrite_venue} WHERE venue_id = %d', $venue_id);
  if ($row = db_fetch_array($result)) {
    return $row;
  }
}

/*
 * Venue Edit Form
 */
function eventbrite_venue($form_state, $venue_id = NULL) {
  $form = array();

  if (isset($venue_id) && $venue = eventbrite_venue_load($venue_id)) {
    $form = eventbrite_venue_form($venue);
  
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Update venue information'),
      '#weight' => 10,
    );
  }
  else {
    $form = eventbrite_venue_form();

    // Organizer field required for new venues
    module_load_include('inc', 'eventbrite', 'eventbrite.organizer');
    $form['organizer'] = array(
      '#type' => 'select',
      '#title' => 'Organizer',
      '#options' => eventbrite_organizer_get_enabled(),
      '#default_value' => variable_get('eventbrite_organizer_default', ''),
      '#required' => 1,
      '#weight' => 9,
    );

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Create this venue'),
      '#weight' => 10,
    );
  }
  return $form;
}

/*
 * Venue Edit Form Validate
 */
function eventbrite_venue_validate($form, &$form_state) {
  $form_values = $form_state['values'];

  $form_errors = eventbrite_venue_form_validate($form_values);
  
//  TODO: Check for form errors

  return;
}

/*
 * Venue Edit Form Submit
 */
function eventbrite_venue_submit($form, &$form_state) {
  $venue = _eventbrite_populate_venue_from_form($form_state['values']);
  if (eventbrite_venue_save($venue)) {
  	// Venue successfully saved
  	return;
  }
  // TODO: Build this error message out more & set a form error if still possible
  drupal_set_message('The venue could not be saved', 'error');
}

function eventbrite_venue_options($required = FALSE) {
  $result = db_query('SELECT * FROM {eventbrite_venue}');
  $venue_options = array();
  if (!$required) {
    $venue_options[''] = '--';
  }
  while ($row = db_fetch_object($result)) {
    $venue_options[$row->venue_id] = $row->name;
  }
  return $venue_options;
}

/*
function eventbrite_venue_list() {
  // TODO: Get venues from Eventbrite server
  $venues = eventbrite_get('user_list_venues');

  if ($venues) {
    $header = array('Venue ID', 'Edit link', 'Venue Name', 'Address' / * TODO: Add venue editing, 'Edit Link' * /);
    $rows = array();
    foreach ($venues as $venue) {
      $address = $venue->address;
      if (!empty($venue->address2)) {
        $address .= ', '. $venue->address2;
      }
      if (!empty($venue->city)) {
        $address .= ', '. $venue->city;
      }
      if (!empty($venue->region)) {
        $address .= ', '. $venue->region;
      }
      if (!empty($venue->country)) {
        $address .= ', '. $venue->country;
      }
      if (!empty($venue->postal_code)) {
        $address .= ', '. $venue->postal_code;
      }
      $rows[] = array(
        $venue->venue_id,
        l('edit', 'admin/settings/eventbrite/venues/'.$venue->venue_id),
//        l($venue->name, '/admin/settings/eventbrite/view-venue/'. $venue->venue_id), 
        $venue->name,
        $address,
        // TODO: Add venue editing back in
//        l('edit', 'admin/settings/eventbrite/list-venues/'. $venue->venue_id),
      );
    }
    return theme('table', $header, $rows);
  }
  
  return t('There are currently no venues conifigured for this user account.');
}
*/
function eventbrite_venue_save($venue) {
  //  Check to see if this venue already exists on Eventbrite server by venue_id
  if (isset($venue->venue_id) && !empty($venue->venue_id)) {
  	// This venue already has a venue_id, so venue_update is the API call
    if ($saved_result = eventbrite_save('venue_update', $venue)) {
      drupal_set_message(t('This venue has been updated on the Eventbrite server.'));
    }
    else {
      drupal_set_message(t('This venue could not be updated on the Eventbrite server'), 'error');
      return;
    }
  }
  else {
    // This venue does not exist, so venue_new is the API call
    if ($saved_result = eventbrite_save('venue_new', $venue)) {
      drupal_set_message(t('This venue has been created on the Eventbrite server.'));
    }
    else {
      drupal_set_message(t('This venue could not be created on the Eventbrite server'), 'error');
      return;
    }
  }
  if ($saved_result) {
  	eventbrite_venue_save_local($venue);
  	return $saved_result;
  }
}

function eventbrite_venue_save_local($venue) {
  // Only save locally if there is already an Eventbrite Venue ID
  if (isset($venue->venue_id) && !empty($venue->venue_id)) {
  	$result = db_query(
      'SELECT * FROM {eventbrite_venue} WHERE venue_id = %d',
      $venue->venue_id
    );
    if ($row = db_fetch_object($result)) {
      // This event already exists in the local DB, so update it
      return drupal_write_record('eventbrite_venue', $venue, array('venue_id'));
    }
    else {
      // This venue does not exist in the local DB, so insert it
      // Set status here
      if (empty($venue->name) || $venue->name == 'TBD' || empty($venue->city)) {
	$venue->status = 0;
      }
      else {
	$venue->status = 1;
      }
      return drupal_write_record('eventbrite_venue', $venue);
    }
  }
}


/*
 * This function pulls venue from the form results and builds a venue object for base Eventbrite module
 */
function _eventbrite_populate_venue_from_form($item) {
	// TODO: All this function does is converts array to object
  $venue = new stdClass();

  if (!empty($item['venue_id'])) {
    $venue->venue_id = $item['venue_id'];
  }
  $venue->name = $item['name'];
  if (!empty($item['organizer'])) {
    $venue->organizer = $item['organizer'];
  }
  $venue->address = $item['address'];
  $venue->address2 = $item['address2'];
  $venue->city = $item['city'];
  $venue->region = $item['region'];
  $venue->postal_code = $item['postal_code'];
  $venue->country = $item['country'];
  $venue->country_code = $item['country'];

  return $venue;
}

function eventbrite_venue_get_enabled() {
  // If Eventbrite OG is enabled and there is a group context
  // TODO: This filtering of venues by sub modules should really happen via a hook
  if (module_exists('eventbrite_og') && $group_node = og_get_group_context()) {
    $filter_venues = eventbrite_og_get_enabled_venues($group_node);
  }
  else {
    // If Eventbrite OG is not overriding this field, use default user key for filtering
    $default_venues = eventbrite_get('user_list_venues');
    $filter_venues = array_keys($default_venues);
  }

  $enabled = array();
  $result = db_query('SELECT venue_id, name, address, address2, city, region FROM {eventbrite_venue} WHERE status = 1 ORDER BY name');
  while ($row = db_fetch_object($result)) {
    if (in_array($row->venue_id, $filter_venues)) {
      $enabled[$row->venue_id] = $row->name;
      if (!empty($row->address)) {
        $enabled[$row->venue_id] .= ', '. $row->address;
      }
      if (!empty($row->address2)) {
        $enabled[$row->venue_id] .= ', '. $row->address2;
      }
      if (!empty($row->city)) {
        $enabled[$row->venue_id] .= ', '. $row->city;
      }
      if (!empty($row->region)) {
        $enabled[$row->venue_id] .= ', '. $row->region;
      }
    }
  }
  return $enabled;
}

/* Enabling venues by city not currently supported
function eventbrite_venue_get_enabled_bycity($list) {
  $enabled = array();
  $result = db_query('SELECT venue_id, city, region FROM {eventbrite_venue} WHERE status = 1 ORDER by city, region');
  while ($row = db_fetch_object($result)) {
    $enabled[$row->city .', '. $row->region][] = $row->venue_id;
  }
  $enabled_bycity = array();
  foreach ($enabled as $city => $venue_ids) {
    $enabled_bycity[serialize($venue_ids)] = $city;
  }
  return $enabled_bycity;
}
*/

function eventbrite_venue_list_form($form_state, $user_key = '') {
  // Set default user key to site wide Eventbrite user key
  if (empty($user_key)) {
    $user_key = variable_get('eventbrite_user_key', '');
    if (empty($user_key)) {
      return t('No Eventbrite user key has been configured for this account.');
    }
  }

  $form = array();

  // This refreshes organizers for the site-wide account
  if ($venues = eventbrite_get('user_list_venues')) {
    $countries_api = countries_api_get_array('iso2', 'printable_name');

    $header = array(
      array(),
      array('data' => t('Venue Name'), 'field' => 'ev.name'),
      array('data' => t('Address'), 'field' => 'ev.address'),
      array('data' => t('City'), 'field' => 'ev.city'),
      array('data' => t('Region / Postal Code'), 'field' => 'ev.region'),
      array('data' => t('Country'), 'field' => 'ev.country'),
    );

    $sql = 'SELECT ev.venue_id, ev.name, ev.address, ev.address2, ev.city, ev.region, ev.postal_code, ev.country FROM {eventbrite_venue} ev WHERE user_key = "'. $user_key .'"';
    $query_count = 'SELECT COUNT(ev.venue_id) FROM {eventbrite_venue} ev WHERE user_key = "'. $user_key .'"';

    $sql .= tablesort_sql($header);

    $result = pager_query($sql, 50, 0, $query_count);

//    $destination = drupal_get_destination();

    $status = array(t('blocked'), t('active'));
    $roles = user_roles(TRUE);
    $venue_array = array();
    while ($venue = db_fetch_object($result)) {
      $venue_array[$venue->venue_id] = '';

      $form['name'][$venue->venue_id] = array(
        '#value' => l(
          $venue->name,
          'admin/settings/eventbrite/venues/'.$venue->venue_id,
          array('query' => $destination)
        ),
      );
      $form['address'][$venue->venue_id] = array('#value' => $venue->address);
      if (!empty($venue->address2)) {
        $form['address'][$venue->venue_id]['#value'] .= '<br/>'. $venue->address2;
      }
      $form['city'][$venue->venue_id] = array('#value' => $venue->city);
      $form['region'][$venue->venue_id] = array('#value' => $venue->region);
      if (!empty($venue->postal_code)) {
        $form['region'][$venue->venue_id]['#value'] .= '<br/>'. $venue->postal_code;
      }
      $form['country'][$venue->venue_id] = array('#value' => $countries_api[$venue->country]);
    }

    $enabled_venues = eventbrite_venue_get_enabled();
    $form['enabled_venues'] = array(
      '#type' => 'checkboxes',
      '#options' => $venue_array,
      '#default_value' => array_keys($enabled_venues),
    );
    
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save enabled venues'),
      '#weight' => 100,
    );
    $form['pager'] = array('#value' => theme('pager', NULL, 50, 0));
  }
  
  return $form;
}

function eventbrite_venue_list_form_submit($form, &$form_state) {
  $enabled_venues = $form_state['values']['enabled_venues'];
  
  // Get current status for all venues in DB
  $db_venue_status = array();
  // Make sure DB matches enabled venues list
  $result = db_query('SELECT venue_id, status FROM {eventbrite_venue} ev WHERE 1');
  while ($venue = db_fetch_object($result)) {
    $db_venues[$venue->venue_id] = $venue;
  }

  // Scroll through venues returned by form.  Since this was a pager query we don't
  //   want to affect any venues that weren't listed on this pager page
  foreach ($enabled_venues as $venue_id => $form_venue_status) {
    $form_venue_status = ($form_venue_status) ? 1 : 0;
    // For each venue, if status has changed update the DB
    $db_venue = $db_venues[$venue_id];
    if ($form_venue_status != $db_venue->status) {
      $db_venue->status = $form_venue_status;
      drupal_write_record('eventbrite_venue', $db_venue, array('venue_id'));
      // TODO: Log to watchdog if this doesn't write
    }
  }
}


/**
 * Theme venue list form.
 *
 * @ingroup themeable
 */
function theme_eventbrite_venue_list_form($form) {
  $header = array(
    theme('table_select_header_cell'),
    array('data' => t('Venue Name'), 'field' => 'ev.name'),
    array('data' => t('Address'), 'field' => 'ev.address'),
    array('data' => t('City'), 'field' => 'ev.city'),
    array('data' => t('Region / Postal Code'), 'field' => 'ev.region'),
    array('data' => t('Country'), 'field' => 'ev.country'),
  );
  
  $output = drupal_render($form['options']);
  if (isset($form['name']) && is_array($form['name'])) {
    foreach (element_children($form['name']) as $key) {
      $rows[] = array(
        drupal_render($form['enabled_venues'][$key]),
        drupal_render($form['name'][$key]),
        drupal_render($form['address'][$key]),
        drupal_render($form['city'][$key]),
        drupal_render($form['region'][$key]),
        drupal_render($form['country'][$key]),
      );
    }
  }
  else {
    $rows[] = array(array('data' => t('No venues were found in this Eventbrite account.'), 'colspan' => '7'));
  }

  $output .= '<div class="form-item"><label>'. t('Enabled Venues:') .'</label></div>';
  $output .= theme('table', $header, $rows);
  if ($form['pager']['#value']) {
    $output .= drupal_render($form['pager']);
  }

  $output .= drupal_render($form);

  return $output;
}