<?php

/**
 * @file
 */

/**
 * Creates nodes from feed items.
 * Note that process, clear gets done in the parent and we simply make sure we
 * do the product stuff here where necessary.  Delete calls the nodeapi delete, so we don't
 * worry about that.
 */
class UCFeedsProductProcessor extends FeedsNodeProcessor {

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    $types = uc_product_types();
    foreach ($types as $t) {
      $options[$t] = $t;
    }
    $type = isset($options['product']) ? 'product' : key($options);
    return array(
      'content_type' => $type,
      'input_format' => FILTER_FORMAT_DEFAULT,
      'update_existing' => FEEDS_SKIP_EXISTING,
      'expire' => FEEDS_EXPIRE_NEVER,
      'mappings' => array(),
      'author' => 0,
      'flatrate_mid' => 0,
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $types = uc_product_types();
    foreach ($types as $t) {
      $options[$t] = $t;
    }
//    $form = parent::configForm(&$form_state);
    $form = parent::configForm(&$form_state);

    //Only allow ubercart product types
    $form['content_type']['#options'] = $options;

    //Add any product specific settings
    if (module_exists('uc_flatrate')) {
      $methods = array(0 => '--None--');
      $q = db_query("SELECT mid, title FROM {uc_flatrate_methods}");
      while ($t = db_fetch_object($q)) {
        $methods[$t->mid] = $t->title;
      }
      $form['flatrate_mid'] = array(
        '#type' => 'select',
        '#title' => t('Flat rate shipping'),
        '#description' => t('Select the flat rate shipping type that is being populated.'),
        '#options' => $methods,
        '#default_value' => $this->config['flatrate_mid'],
      );
    }
    return $form;
  }

  /**
   * Reschedule if expiry time changes.
   */
  public function configFormSubmit(&$values) {
    parent::configFormSubmit($values);
  }

  /**
   * Override setTargetElement to operate on a target item that is a node.
   */
  public function setTargetElement($target_node, $target_element, $value) {
    // Set the target elements on the product.
    $target_elements = array(
      'model',
      'list_price',
      'cost',
      'sell_price',
      'weight',
      'weight_units',
      'pkg_qty',
      'dim_length',
      'dim_width',
      'dim_height',
      'length_units',
      'default_qty',
      'shippable',
      'ordering',
      'shipping_type',
    );
    if (in_array($target_element, $target_elements)) {
      $target_node->$target_element = $value;
    }
    else if ($target_element == 'flatrate' && $this->config['flatrate_mid'] && intval($value) > 0) {
      // This needs to be stored as an array of $mid=>$rate.  Mid comes from config.
      $target_node->$target_element = array($this->config['flatrate_mid'] => $value);
    }
    else {
      // If none of these then pass to the parent node.
      parent::setTargetElement($target_node, $target_element, $value);
    }
  }

  /**
   * Return available mapping targets.
   */
  public function getMappingTargets() {
    // We'll add uc product targets to this.
    $targets = parent::getMappingTargets();
    $targets += array(
      'model' => array(
        'name' => t('UC: Model/SKU'),
        'description' => t('Ubercart: Model/SKU'),
        'optional_unique' => TRUE,
       ),
      'list_price' => array(
        'name' => t('UC: List price'),
        'description' => t('Ubercart: List price'),
       ),
      'cost' => array(
        'name' => t('UC: Cost'),
        'description' => t('Ubercart: Cost'),
      ),
      'sell_price' => array(
        'name' => t('UC: Sell price'),
        'description' => t('Ubercart: Sell price'),
      ),
      'weight' => array(
        'name' => t('UC: Weight'),
        'description' => t('Ubercart: Weight'),
      ),
      'weight_units' => array(
        'name' => t('UC: Weight Units of measurement'),
        'description' => t('Ubercart: Weight Units of Measurement'),
      ),
      'pkg_qty' => array(
        'name' => t('UC: Package Quantity'),
        'description' => t('Ubercart: Package Quantity'),
      ),
      'dim_length' => array(
        'name' => t('UC: Dim Length'),
        'description' => t('Ubercart: Dim Length'),
      ),
      'dim_width' => array(
        'name' => t('UC: Dim Width'),
        'description' => t('Ubercart: Dim Width'),
      ),
      'dim_height' => array(
        'name' => t('UC: Dim Height'),
        'description' => t('Ubercart: Dim Height'),
      ),
      'length_units' => array(
        'name' => t('UC: Dimension Units of measurement'),
        'description' => t('Ubercart: Dimension Units Of Measurement'),
      ),
      'default_qty' => array(
        'name' => t('UC: Default Quantity'),
        'description' => t('Ubercart: Default Quantity to add to cart'),
      ),
      'shippable' => array(
        'name' => t('UC: Shippable'),
        'description' => t('Ubercart: Shippable'),
      ),
      'ordering' => array(
        'name' => t('UC: List Position'),
        'description' => t('Ubercart: List Position'),
      ),
      'shipping_type' => array(
        'name' => t('UC: Default product shipping type'),
        'description' => t('Ubercart: Default product shipping type'),
      ),
    );

    // Stock.
    if (module_exists("uc_stock")) {
      $targets['stock'] = array(
        'name' => t('UC: Stock Level'),
        'description' => t('Ubercart: Stock Level'),
      );
    }
    // TODO : Price By Role, others?
    // Attributes.
    if (module_exists("uc_attribute")) {
      // CC Only supported after UC 2.5 uc_attribute_load_multiple().
      $attribs = array();
      foreach ($attribs as $attrib) {
        $aid = $attrib->aid;
        foreach ($attrib->options as $option) {
          $oid = $option->oid;
          $targets['attribute_price_' . $aid . "_" . $oid] = array(
            'name' => t('UCA Price: ' . $attrib->name . ":" . $option->name),
            'description' => t('Ubercart: Attribute Price:' . $attrib->name . ': ' . $option->name),
          );
          $targets['attribute_weight_' . $aid . "_" . $oid] = array(
            'name' => t('UCA Weight: ' . $attrib->name . ': ' . $option->name),
            'description' => t('Ubercart: Attribute Weight: ' . $attrib->name . ': ' . $option->name),
          );
        }
      }
    }
    // Shipping uc_flatrate.
    if (module_exists("uc_flatrate") && $this->config['flatrate_mid']) {
      $targets['flatrate'] = array(
        'name' => t('UC: Flatrate Shipping'),
        'description' => t('Ubercart: Flatrate Shipping'),
      );
    }
    return $targets;
  }

  /**
   * Get nid of an existing feed item node if available.
   */
  protected function existingItemId(FeedsImportBatch $batch, FeedsSource $source) {
    //First see if the node is found in the unique targets.
    $nid = parent::existingItemId($batch, $source);
    if ($nid) {
      return $nid;
    }
    else {
      // Look for a match with the 'model/sku'. Iterate through all unique
      // targets and test whether they already exist in the database.
      foreach ($this->uniqueTargets($batch) as $target => $value) {
        switch ($target) {
          case 'model':
            $nid = db_result(db_query("SELECT nid FROM {uc_products} WHERE model = '%s'", $value));
            break;
        }
        if ($nid) {
          // Return with the first nid found.
          return $nid;
        }
      }
    }
    return 0;
  }
}
