<?php
/**
* @file
* Quick form for adding weights
*/

function farm_quickweight_farm_quick_forms() {
  return array(
    'quickweight' => array(
      'label' => t('Weight'),
      'permission' => 'create farm_observation log entities',
      'form' => 'farm_quickweight_form',
      'file' => 'farm_quickweight.farm_quick.inc',
    ),
  );
}

function farm_quickweight_form($form, &$form_state){
	$system = farm_quantity_system_of_measurement();

	$default_units = 'kg';
	if ($system == 'us'){
		$default_units = 'lbs';
	}
	$default_measure = variable_get('farm_quick_weight_measure',$default_measure);
	$default_units = variable_get('farm_quick_weight_units', $default_units);

	$form['weight'] = array(
	'#type'=> 'fieldset',
	'#title' => t('Record animal weights'),
	'#description' => t('Use this form to record animal weights. An observation log will be created with the details filled in.'),
	);

	$form['weight']['timestamp'] = array(
    '#type' => 'date_select',
    '#title' => t('Date'),
    '#date_format' => 'M j Y H:i',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#default_value' => REQUEST_TIME,
    '#required' => TRUE,
  );

	$form['weight']['asset'] = array(
	'#type'=>'textfield',
	'#title' => t('Group/animal'),
	'#description' => t('Select the group or animal the weight is for'),
	'#autocomplete_path' => 'farm_asset/autocomplete/animal+group',
    '#required' => TRUE,
	);


	$form['weight']['quantity'] = array(
	'#type' => 'textfield',
	'#title' => t('Quantity'),
	'#required'=> TRUE,
	'#element_validate' => array('element_validate_number'),
	);

	$form['weight']['units'] = array(
	'#type' => 'textfield',
	'#title' => t('Unit of measurement'),
	'#description' => t('Specify the units you weighed in'),
	'#autocomplete_path' => 'taxonomy/autocomplete/field_farm_quantity_units',
	'#default_value' => $default_units,
	'#required' => TRUE,
	);

	$form['weight']['submit'] = array(
	'#type' => 'submit',
	'#value' => t('Save weight'),
  '#suffix' => '<br>',
	);

  $form['weight']['previous'] = array(
    '#type' => 'fieldset',
    '#title' => t("Previous Weights"),
    '#prefix' => '<br>',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  $assetList = entity_load('farm_asset');

  foreach ($assetList as $asset){
    if ($asset->archived == 0 && ($asset->type =='animal' || $asset->type == 'group' )){
      $weight = farm_quickweight_getLast2Weights($asset);
      if (!empty($weight)){

        $dlwg = farm_quickweight_dlwg($weight);
      $form['weight']['previous'][$weight['name']] = array(
        '#markup' => '<strong>'.$weight['name'] . ': </strong>' . $dlwg,
        '#prefix' => '<br>'
      );

      }

    }
  }





  // $form['weight']['previous'][''];

	return $form;
}
function farm_quickweight_form_validate($form, &$form_state){

	if (!empty($form_state['values']['asset'])){
		$id = 0;
		$matches = array();
		$result = preg_match('/\\[id: ([0-9]+)\\]/', $form_state['values']['asset'], $matches);
		if (!empty($matches[$result])){
			$id=$matches[$result];
		}
	if (empty($id)) {
      form_set_error('asset', t('Could not load the animal/group record. Make sure the asset ID is included. For example: "My animal [id: 123]"'));
    }

    // Load the asset.
    $asset = farm_asset_load($id);

    // If the asset didn't load, throw an error.
    if (empty($asset)) {
      form_set_error('asset', t('Could not load the animal/group record. Make sure the asset name and ID are correct.'));
    }

    // Save the asset to the form state.
    $form_state['storage']['asset'] = $asset;
  };
}

function farm_quickweight_form_submit($form, &$form_state){
	$qty = $form_state['values']['quantity'];
  $units = $form_state['values']['units'];
	variable_set('farm_quick_weight_measure',$qty);
	variable_set('farm_quick_weight_units',$units);
	$timestamp = strtotime($form_state['values']['timestamp']);
	$asset = $form_state['storage']['asset'];
	$assets = array($asset);

  farm_livestock_weight_set($assets,$qty,$units, $timestamp);
	if(function_exists('farm_quick_entity_link')){
		farm_quick_entity_link('farm_quickweight_form', 'log', $log);
	}
}

function farm_quickweight_getLast2Weights($asset){
  if (empty($asset->id)) {
  return FALSE;
}
  $weight_list = array();
  $query = farm_quantity_log_asset_query($asset->id,'weight', NULL, REQUEST_TIME, TRUE, 'farm_observation', FALSE);
  $query->orderBy('ss_log.timestamp', 'DESC');
  $query->orderBy('ss_log.id', 'DESC');
  $query->range(0,2);
  $results = $query->execute();
  $resultIDs = $results->fetchCol();
  if (count($resultIDs) > 1){
    $weight_list['id'] = $asset->id;
    $weight_list['name'] = $asset->name;
    foreach($resultIDs as $logIDs){
      $log = log_load($logIDs);
      $log_data = farm_quantity_log_data($log);
      $weight_list[] = array('id'=>$logIDs, 'data'=>$log_data, 'timestamp'=>$log->timestamp);

    }
    return $weight_list;
  }else{
    return FALSE;
  }

}

function farm_quickweight_dlwg($weights){
  if (empty($weights)){
    return FALSE;
  }
  if ($weights[0]['data'][0]['units'] == $weights[1]['data']['0']['units']) {
    //Check units are the same
    $units = $weights[1]['data']['0']['units'];
    $latest_weight = $weights[0];
    $previous_weight = $weights[1];
    $weigh_difference = $latest_weight['data'][0]['value'] - $previous_weight['data'][0]['value'];
    $time_difference = $latest_weight['timestamp'] - $previous_weight['timestamp'];
    $time_difference_days = round($time_difference/86400,2);
    $dlwg = round($weigh_difference/$time_difference_days,3);

    $date_format = 'D  d M Y';
    $latest_log = log_load($latest_weight['id']);
    $latest_log_uri= entity_uri('log', $latest_log);
    $latest_log_date = date($date_format,$latest_weight['timestamp']);

    $previous_log = log_load($previous_weight['id']);
    $previous_log_uri = entity_uri('log',$previous_log);
    $previous_log_date = date($date_format,$previous_weight['timestamp']);

    $dlwg_text = $dlwg . ' ' . $units . '/' . t('Day');

    $observed_text = t(
            'observed between <a href="!previous_log_link">@previous_log_date</a> and <a href="!latest_log_link">@latest_log_date</a>',
            array(
              '!previous_log_link' => url($previous_log_uri['path']),
              '@previous_log_date' => $previous_log_date,
              '!latest_log_link' => url($latest_log_uri['path']),
              '@latest_log_date' => $latest_log_date)
          );

    $markup = '<p><strong>' . t('Daily liveweight gain') . ':</strong> ' . $dlwg_text . ' (' . $observed_text . ')</p><br>';
    return $markup;

  }
}
