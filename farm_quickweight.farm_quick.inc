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
	);
	
	return $form;
}
function farm_quickweight_form_validate($form, &$form_state){
	
	if (!empty($form_state['values']['asset'])){
		$id = 0;
		$matches = array();
		$result = preg_match('/\\[id: ([0-9]+)\\]$/', $form_state['values']['asset'], $matches);
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
	$measure = 'weight';
	$units = check_plain($form_state['values']['units']);
	variable_set('farm_quick_weight_measure',$measure);
	variable_set('farm_quick_weight_units',$units);
	
	$asset = $form_state['storage']['asset'];
	$assets = array($asset);
	$log_type = 'farm_observation';
	
	$log_name = t('Weight of @asset: @qty @units', array('@asset' => entity_label('farm_asset', $asset), '@qty'=> $form_state['values']['quantity'], '@units' => $form_state['values']['units']));
	
	$measurements = array(
	array(
	'measure' => $measure,
	'value' => $form_state['values']['quantity'],
	'units' => $units,
	'label' => t('Weight'),
	),
	);
	
	$log = farm_quantity_log_create($log_type,$log_name, REQUEST_TIME, TRUE, $assets, $measurements);
	
	if(function_exists('farm_quick_entity_link')){
		farm_quick_entity_link('farm_quickweight_form', 'log', $log);
	}
}