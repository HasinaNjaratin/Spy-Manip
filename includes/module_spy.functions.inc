<?php

/**
* Log manip
**/
function module_spy_log_manip($change){
  global $user;
  db_insert('spy')->fields(array(
      'spy_Id' => REQUEST_TIME,
      'uid' => $user->uid,
      'ip' => ip_address(),
      'time' => REQUEST_TIME,
      'url' => current_path(),
      'form_id' => $change['form'],
      'changed' => $change['change'],
  ))->execute();
}


/*
 * Pass both versions of the node to this function. Returns an array of
 * fields that were changed or an empty array if none were changed.
 * Pass field collections as an array keyed by field collection ID.
 *
 * @param object $old_entity
 *   The original (stored in the database) node object.
 *   This function may also pass itself a FieldCollectionItemEntity object to compare field collections.
 * @param object $new_entity
 *   The prepared node object for comparison.
 *   This function may also pass itself a FieldCollectionItemEntity object to compare field collections.
 */
function module_spy_fields_changed_node($old_entity, $new_entity, $bundle=NULL) {
  // Check for node or field collection.
  $entity_is_field_collection = (get_class($old_entity) == 'FieldCollectionItemEntity');

  $bundle = ($entity_is_field_collection ? $old_entity->field_name : $old_entity->type);

  // Sanity check. Exit and throw an error if the content types don't match.
  if ($bundle !== ($entity_is_field_collection ? $new_entity->field_name : $new_entity->type)) {
    drupal_set_message('Content type mismatch. Unable to save changes.', 'error');
    return FALSE;
  }

  // Get field info.
  $field_read_params = array(
    /*'entity_type' =>*/ /*($entity_is_field_collection ? 'field_collection_item' :*/ /*'taxonomy_term'*/
    'bundle' => $bundle
  );
  $fields_info = field_read_fields($field_read_params);

  $fields_changed = array();

  foreach ($fields_info as $field_name => $field_info) {
    $old_field = $old_entity->$field_name;
    $new_field = $new_entity->$field_name;

    // Check the number of values for each field, or if they are populated at all.
    $old_field_count = (isset($old_field[LANGUAGE_NONE]) ? count($old_field[LANGUAGE_NONE]) : 0);
    $new_field_count = (isset($new_field[LANGUAGE_NONE]) ? count($new_field[LANGUAGE_NONE]) : 0);

    if ($old_field_count != $new_field_count) {
      // The two versions have a different number of values. Something has changed.
      $fields_changed[] = $field_name;
    }
    elseif ($old_field_count > 0 && $new_field_count > 0) {
      // Both versions have an equal number of values. Time to compare.

      // See if this field is a field collection.
      if ($field_info['type'] == 'field_collection') {

        foreach ($new_field[LANGUAGE_NONE] as $delta => $values) {
          $old_field_collection = entity_load_unchanged('field_collection_item', $values['entity']->item_id);
          $new_field_collection = $values['entity'];

          $fields_changed = array_merge($fields_changed, _fields_changed($old_field_collection, $new_field_collection));
        }
        unset($delta, $values);

      }
      else {
        foreach ($old_field[LANGUAGE_NONE] as $delta => $value) {
          foreach ($field_info['columns'] as $field_column_name => $field_column_info) {
            $old_value = $old_field[LANGUAGE_NONE][$delta][$field_column_name];
            $new_value = $new_field[LANGUAGE_NONE][$delta][$field_column_name];
            $field_column_type = $field_column_info['type'];

            // As with the overall field, exit if one version has a value and the other doesn't.
            if (isset($old_value) != isset($new_value)) {
              $fields_changed[] = $old_field;
            }
            elseif (isset($old_value) && isset($new_value)) {
              // The column stores numeric data so compare values non-identically.
              if (in_array($field_column_type, array(
                'int',
                'float',
                'numeric'
              ))) {
                if ($new_value != $old_value) {
                  $fields_changed[] = $field_name;
                }
              }
              // The column stores non-numeric data so compare values identically,
              elseif ($new_value !== $old_value) {
                $fields_changed[] = $field_name;
              }
            }
            else {
              // Included for clarity. Both values are empty so there was obviously no change.
            }
          }
          unset($field_column_name, $field_column_info);
        }
        unset($delta, $value);
      }
    }
    else {
      // Included for clarity. Both values are empty so there was obviously no change.
    }
  }
  unset($field_name, $field_info);
  // End of field comparison loop.

  return $fields_changed;
}

/*
* Fields changed from taxonomy term
*/
function module_spy_fields_changed_term($old_term, $new_term){
  $fields_changed = [];
  if($old_term->name !== $new_term->name){ $fields_changed[] = 'name'; }
  if($old_term->description !== $new_term->description){ $fields_changed[] = 'description'; }
  foreach ($old_term as $key => $value) {
    if ((strpos($key, 'field_') !== false) && (isset($old_term->{$key}) && !empty($old_term->{$key}) && (isset($new_term->{$key})) && !empty($new_term->{$key}) )) {
      if(!empty($old_term->{$key}) && (sizeof($old_term->{$key}) == sizeof($new_term->{$key})) ){
        foreach ($old_term->{$key}[LANGUAGE_NONE] as $id => $valeur) {
          reset($valeur);
          $get_value =  key($valeur);
          if($old_term->{$key}[LANGUAGE_NONE][$id][$get_value] != $new_term->{$key}[LANGUAGE_NONE][$id][$get_value]){
            $fields_changed[] = $key;
          }
        }
      }else{
        $fields_changed[] = $key;
      }
    }
  }
  return $fields_changed;
}

/*
* Fields changed from user
*/
function module_spy_fields_changed_user($edit,$account){
  $fields_changed = [];
  return $fields_changed;
}