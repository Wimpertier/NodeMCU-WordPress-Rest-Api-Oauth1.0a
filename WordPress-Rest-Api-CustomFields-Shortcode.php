<?php
/**
 * Plugin Name: Wimpertier Rest-Api Shortcode
 * Description: Add custom fields to the WordPress Rest-Api and output them with a shortcode
 * Version:     20190521
 * Author:      Simone Truniger
 * Author URI:  https://www.wimpertier.net/
 * License:     GPL2
 * License URI: https://www.gnu.org/licenses/gpl-2.0.html
 */

//Adding custom fields to the WordPress Rest-Api
add_action( 'rest_api_init', 'wt_register_posts_meta_field' );

function wt_register_posts_meta_field() { 
    
    $object_type = 'post';
    
    $args1 = array( 
        'type'          => 'string',
        'single'        => true,
        'show_in_rest'  => true,
    );
    register_meta( $object_type, 'wt_soil_temperature', $args1 );

    $args2 = array( 
        'type'          => 'string',
        'single'        => true,
        'show_in_rest'  => true,
    );
    register_meta( $object_type, 'wt_timestamp', $args2 );
}

//Create shortcode [soil_temperature]
add_shortcode( 'soil_temperature', 'wt_soil_temperature');

function wt_soil_temperature() {

    $wt_soil_temperature = get_post_meta( get_the_ID(), 'wt_soil_temperature', true );

    $wt_timestamp = get_post_meta( get_the_ID(), 'wt_timestamp', true );
    $wt_timestamp = date( 'Y-m-d H:i', $wt_timestamp );
	
    ob_start();
    ?>
    <table>
        <tr>
            <td colspan="2">Facts about the soil termometer</td>
        </tr><tr>
            <td>Soil temperature: </td>
            <td><?php echo $wt_soil_temperature; ?></td>
        </tr><tr>
            <td>Last measurement: </td>
            <td><?php echo $wt_timestamp; ?></td>
        </tr>
    </table>
    <?php
    return ob_get_clean();
}
?>