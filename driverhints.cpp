/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "Driver.h"
#include "Globals.h"

void
TController::hint( driver_hint const Value, hintpredicate const Predicate, float const Predicateparameter ) {

    if( Predicate( Predicateparameter ) ) { return; }
    // check already recorded hints, if there's a match update it instead of adding a new one
    for( auto &hint : m_hints ) {
        if( std::get<driver_hint>( hint ) == Value ) {
            std::get<float>( hint ) = Predicateparameter;
            return; // done, can bail out early
        }
    }
    m_hints.emplace_back( Value, Predicate, Predicateparameter );
}

void
TController::update_hints() {

    m_hints.remove_if(
        []( auto const &Hint ) {
            return ( std::get<hintpredicate>( Hint )( std::get<float>( Hint ) ) ); } );
}

void
TController::remove_hint( driver_hint const Value ) {

    m_hints.remove_if(
        [=]( auto const &Hint ) {
            return ( std::get<driver_hint>( Hint ) == Value ); } );
}

void
TController::remove_train_brake_hints() {

    remove_hint( driver_hint::trainbrakesetpipeunlock );
    remove_hint( driver_hint::trainbrakerelease );
    remove_hint( driver_hint::trainbrakeapply );
    remove_hint( driver_hint::brakingforcedecrease );
    remove_hint( driver_hint::brakingforceincrease );
    remove_hint( driver_hint::brakingforcesetzero );
    remove_hint( driver_hint::brakingforcelap );
}

void
TController::remove_master_controller_hints() {

    remove_hint( driver_hint::mastercontrollersetidle );
    remove_hint( driver_hint::mastercontrollersetseriesmode );
    remove_hint( driver_hint::mastercontrollersetzerospeed );
    remove_hint( driver_hint::mastercontrollersetreverserunlock );
    remove_hint( driver_hint::tractiveforcedecrease );
    remove_hint( driver_hint::tractiveforceincrease );
    remove_hint( driver_hint::bufferscompress );
}

void
TController::remove_reverser_hints() {

    remove_hint( driver_hint::directionforward );
    remove_hint( driver_hint::directionbackward );
    remove_hint( driver_hint::directionother );
    remove_hint( driver_hint::directionnone );
}

void
TController::cue_action( driver_hint const Action, float const Actionparameter ) {

    switch( Action ) {
        // battery
        case driver_hint::batteryon: {
            if( AIControllFlag ) {
                mvOccupied->BatterySwitch( true );
            }
            remove_hint( driver_hint::batteryoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->BatteryStart != start_t::manual ) || ( mvOccupied->Power24vIsAvailable == true ) ); } );
            break;
        }
        case driver_hint::batteryoff: {
            if( AIControllFlag ) {
                mvOccupied->BatterySwitch( false );
            }
            remove_hint( driver_hint::batteryon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->BatteryStart != start_t::manual ) || ( mvOccupied->Power24vIsAvailable == false ) ); } );
            break;
        }
		// battery
		case driver_hint::cabactivation: {
			if (AIControllFlag && mvOccupied->Power24vIsAvailable) {
				mvOccupied->CabActivisation();
				CheckVehicles();
			}
			remove_hint( driver_hint::cabdeactivation );
			hint(
				Action,
				[this](float const Parameter) -> bool {
				return ( ( mvOccupied->AutomaticCabActivation ) || (  mvOccupied->IsCabMaster() ) ); } );
			break;
		}
		case driver_hint::cabdeactivation: {
			if (AIControllFlag) {
				mvOccupied->CabDeactivisation();
			}
			remove_hint( driver_hint::cabactivation );
			hint(
				Action,
				[this](float const Parameter) -> bool {
				return ( ( mvOccupied->AutomaticCabActivation ) || ( ( mvOccupied->CabMaster == false ) && ( mvOccupied->CabActive == 0 ) ) ); } );
			break;
		}
        // radio
        case driver_hint::radioon: {
            if( AIControllFlag ) {
                mvOccupied->Radio = true;
            }
            remove_hint( driver_hint::radiooff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->Radio == true ); } );
            break;
        }
        case driver_hint::radiochannel: {
            if( AIControllFlag ) {
                iRadioChannel = static_cast<int>( Actionparameter );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( iRadioChannel == static_cast<int>( Parameter ) ); },
                Actionparameter );
            break;
        }
        case driver_hint::radiooff: {
            if( AIControllFlag ) {
                mvOccupied->Radio = false;
            }
            remove_hint( driver_hint::radioon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->Radio == false ); } );
            break;
        }
        // oil pump
        case  driver_hint::oilpumpon: {
            if( AIControllFlag ) {
                mvOccupied->OilPumpSwitch( true );
            }
            remove_hint( driver_hint::oilpumpoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->OilPump };
                    return ( ( device.start_type != start_t::manual ) || ( device.is_enabled == true ) || ( device.is_active == true ) || ( mvOccupied->Mains ) ); } );
            break;
        }
        case driver_hint::oilpumpoff: {
            if( AIControllFlag ) {
                mvOccupied->OilPumpSwitch( false );
            }
            remove_hint( driver_hint::oilpumpon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->OilPump };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == false ) && ( device.is_active == false ) ) ); } );
            break;
        }
        // fuel pump
        case driver_hint::fuelpumpon: {
            if( AIControllFlag ) {
                mvOccupied->FuelPumpSwitch( true );
            }
            remove_hint( driver_hint::fuelpumpoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->FuelPump };
                    return ( ( device.start_type != start_t::manual ) || ( device.is_enabled == true ) || ( device.is_active == true ) ); } );
            break;
        }
        case driver_hint::fuelpumpoff: {
            if( AIControllFlag ) {
                mvOccupied->FuelPumpSwitch( false );
            }
            remove_hint( driver_hint::fuelpumpon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->FuelPump };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == false ) && ( device.is_active == false ) ) ); } );
            break;
        }
        // pantographs
        case driver_hint::pantographairsourcesetmain: {
            if( AIControllFlag ) {
                mvPantographUnit->bPantKurek3 = true;
            }
            remove_hint( driver_hint::pantographairsourcesetauxiliary );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->bPantKurek3 == true ); } );
            break;
        }
        case driver_hint::pantographairsourcesetauxiliary: {
            if( AIControllFlag ) {
                mvPantographUnit->bPantKurek3 = false;
            }
            remove_hint( driver_hint::pantographcompressoron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->bPantKurek3 == false ); } );
            break;
        }
        case driver_hint::pantographcompressoron: {
            if( AIControllFlag ) {
                mvPantographUnit->PantCompFlag = true;
            }
            remove_hint( driver_hint::pantographcompressoroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->PantCompFlag == true ); } );
            break;
        }
        case driver_hint::pantographcompressoroff: {
            if( AIControllFlag ) {
                mvPantographUnit->PantCompFlag = false;
            }
            remove_hint( driver_hint::pantographcompressoron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->PantCompFlag == false ); } );
            break;
        }
        case driver_hint::pantographsvalveon: {
            if( AIControllFlag ) {
                mvOccupied->OperatePantographsValve( operation_t::enable );
            }
            remove_hint( driver_hint::pantographsvalveoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->PantsValve.is_active == true ); } );
            break;
        }
        case driver_hint::frontpantographvalveon: {
            if( AIControllFlag ) {
                mvOccupied->OperatePantographValve( end::front, operation_t::enable );
            }
            remove_hint( driver_hint::frontpantographvalveoff );
            hint(
                Action,
                [ this ]( float const Parameter ) -> bool {
                    return ( ( mvPantographUnit->Pantographs[ end::front ].valve.is_active == true ) || ( ( Parameter > 0 ) && ( mvOccupied->Vel > Parameter ) ) ); },
                Actionparameter );
            break;
        }
        case driver_hint::frontpantographvalveoff: {
            if( AIControllFlag ) {
                mvOccupied->OperatePantographValve( end::front, operation_t::disable );
            }
            remove_hint( driver_hint::frontpantographvalveon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->Pantographs[ end::front ].valve.is_active == false ); } );
            break;
        }
        case driver_hint::rearpantographvalveon: {
            if( AIControllFlag ) {
                mvOccupied->OperatePantographValve( end::rear, operation_t::enable );
            }
            remove_hint( driver_hint::rearpantographvalveoff );
            hint(
                Action,
                [ this ]( float const Parameter ) -> bool {
                    return ( ( mvPantographUnit->Pantographs[ end::rear ].valve.is_active == true ) || ( ( Parameter > 0 ) && ( mvOccupied->Vel > Parameter ) ) ); },
                Actionparameter );
            break;
        }
        case driver_hint::rearpantographvalveoff: {
            if( AIControllFlag ) {
                mvOccupied->OperatePantographValve( end::rear, operation_t::disable );
            }
            remove_hint( driver_hint::rearpantographvalveon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->Pantographs[ end::rear ].valve.is_active == false ); } );
            break;
        }
        // converter
        case driver_hint::converteron: {
            if( AIControllFlag ) {
                mvOccupied->ConverterSwitch( true );
            }
            remove_hint( driver_hint::converteroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyConverterEnabled == true ); } );
            break;
        }
        case driver_hint::converteroff: {
            if( AIControllFlag ) {
                mvOccupied->ConverterSwitch( false );
            }
            remove_hint( driver_hint::converteron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyConverterExplicitlyEnabled == false ); } );
            break;
        }
        // relays
        case driver_hint::primaryconverteroverloadreset: {
            if( AIControllFlag ) {
                mvOccupied->RelayReset( relay_t::primaryconverteroverload );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->ConverterOverloadRelayStart != start_t::manual ) || ( mvOccupied->ConvOvldFlag == false ) ); } );
            break;
        }
        case driver_hint::maincircuitgroundreset: {
            if( AIControllFlag ) {
                mvOccupied->RelayReset( relay_t::maincircuitground );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->GroundRelayStart != start_t::manual ) || ( mvOccupied->GroundRelay == true ) ); } );
            break;
        }
        case driver_hint::tractionnmotoroverloadreset: {
            if( AIControllFlag ) {
                mvOccupied->RelayReset( relay_t::tractionnmotoroverload );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->FuseFlag == false ); } );
            break;
        }
        // line breaker
        case driver_hint::linebreakerclose: {
            if( AIControllFlag ) {
                mvOccupied->MainSwitch( true );
            }
            remove_hint( driver_hint::linebreakeropen );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyLineBreakerOpen == false ); } );
            break;
        }
        case driver_hint::linebreakeropen: {
            if( AIControllFlag ) {
                mvOccupied->MainSwitch( false );
            }
            remove_hint( driver_hint::linebreakerclose );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    // TBD, TODO: replace with consist-wide flag set true if any line breaker is closed?
                    return ( mvControlling->Mains == false ); } );
            break;
        }
        // compressor
        case driver_hint::compressoron: {
            if( AIControllFlag ) {
                mvOccupied->CompressorSwitch( true );
            }
            remove_hint( driver_hint::compressoroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyCompressorEnabled == true ); } );
            break;
        }
        case driver_hint::compressoroff: {
            if( AIControllFlag ) {
                mvOccupied->CompressorSwitch( false );
            }
            remove_hint( driver_hint::compressoron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyCompressorExplicitlyEnabled == false ); } );
            break;
        }
        case driver_hint::frontmotorblowerson: {
            if( AIControllFlag ) {
                mvOccupied->MotorBlowersSwitchOff( false, end::front );
                mvOccupied->MotorBlowersSwitch( true, end::front );
            }
            remove_hint( driver_hint::frontmotorblowersoff );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    auto const &device { mvOccupied->MotorBlowers[ end::front ] };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == true ) && ( device.is_disabled == false ) ) ); } );
            break;
        }
        case driver_hint::rearmotorblowerson: {
            if( AIControllFlag ) {
                mvOccupied->MotorBlowersSwitchOff( false, end::rear );
                mvOccupied->MotorBlowersSwitch( true, end::rear );
            }
            remove_hint( driver_hint::rearmotorblowersoff );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    auto const &device { mvOccupied->MotorBlowers[ end::rear ] };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == true ) && ( device.is_disabled == false ) ) ); } );
            break;
        }
        // spring brake
        case driver_hint::springbrakeon: {
            if( AIControllFlag ) {
                mvOccupied->SpringBrakeActivate( true );
            }
            remove_hint( driver_hint::springbrakeoff );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( mvOccupied->SpringBrake.Activate == true ); } );
            break;
        }
        case driver_hint::springbrakeoff: {
            if( AIControllFlag ) {
                mvOccupied->SpringBrakeActivate( false );
            }
            remove_hint( driver_hint::springbrakeon );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( mvOccupied->SpringBrake.Activate == false ); } );
            break;
        }
        // manual brake
        case driver_hint::manualbrakon: {
            if( AIControllFlag ) {
                mvOccupied->IncManualBrakeLevel( ManualBrakePosNo );
            }
            remove_hint( driver_hint::manualbrakoff );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( ( mvOccupied->LocalBrake != TLocalBrake::ManualBrake ) || ( mvOccupied->MBrake == false ) || ( mvOccupied->ManualBrakePos == ManualBrakePosNo ) ); } );
            break;
        }
        case driver_hint::manualbrakoff: {
            if( AIControllFlag ) {
                mvOccupied->DecManualBrakeLevel( ManualBrakePosNo );
            }
            remove_hint( driver_hint::manualbrakon );
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( ( mvOccupied->LocalBrake != TLocalBrake::ManualBrake ) || ( mvOccupied->MBrake == false ) || ( mvOccupied->ManualBrakePos == 0 ) ); } );
            break;
        }
        // master controller
        case driver_hint::mastercontrollersetidle: {
            if( AIControllFlag ) {
                while( ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn == 0 )
                    && ( mvControlling->IncMainCtrl( 1 ) ) ) {
                    ;
                }
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( mvControlling->RList[ mvControlling->MainCtrlPos ].Mn > 0 ); } );
            break;
        }
        case driver_hint::mastercontrollersetseriesmode: {
            if( AIControllFlag ) {
                if( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 ) {
                    // limit yourself to series mode
                    if( mvControlling->ScndCtrlPos ) {
                        mvControlling->DecScndCtrl( 2 );
                    }
                    while( ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn > 1 )
                        && ( mvControlling->DecMainCtrl( 1 ) ) ) {
                        ; // all work is performed in the header
                    }
                }
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this]( float const Parameter ) -> bool {
                    return ( mvControlling->RList[ mvControlling->MainCtrlPos ].Bn < 2 ); } );
            break;
        }
        case driver_hint::mastercontrollersetzerospeed: {
            if( AIControllFlag ) {
                ZeroSpeed();
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->IsMainCtrlNoPowerPos() ) && ( mvControlling->IsScndCtrlNoPowerPos() ) ); } );
            break;
        }
        case driver_hint::mastercontrollersetreverserunlock: {
            if( AIControllFlag ) {
                while( ( false == mvControlling->EIMDirectionChangeAllow() )
                    && ( mvControlling->DecMainCtrl( 1 ) ) ) {
                    ;
                }
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvControlling->EIMDirectionChangeAllow() ); },
                mvControlling->MainCtrlMaxDirChangePos );
            break;
        }
        case driver_hint::tractiveforcedecrease: {
            if( AIControllFlag ) {
                DecSpeed();
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( std::abs( mvControlling->Ft ) <= Parameter ); },
                std::min(
                    0.0,
                    std::max(
                        std::abs( mvControlling->Ft ) * 0.95,
                        std::abs( mvControlling->Ft ) - 5.0 ) ) );
            break;
        }
        case driver_hint::tractiveforceincrease: {
            if( AIControllFlag ) {
                IncSpeed();
            }
            remove_master_controller_hints();
            // clear potentially remaining braking hints
            // TODO: add other brake application hints, refactor into a method
            {
                remove_hint( driver_hint::brakingforceincrease );
                remove_hint( driver_hint::independentbrakeapply );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( AccDesired <= EU07_AI_NOACCELERATION )
                          || ( ( false == Ready ) && ( false == mvOccupied->ShuntMode ) )
                          || ( AccDesired - AbsAccS <= 0.05 )
                          || ( mvOccupied->EIMCtrlType > 0 ?
                               ( mvControlling->eimic_real >= 1.0 ) :
                               ( ( mvControlling->IsScndCtrlMaxPowerPos() ) && ( mvControlling->IsMainCtrlMaxPowerPos() ) ) ) ); } );
            break;
        }
        case driver_hint::bufferscompress: {
            if( AIControllFlag ) {
                if( std::abs( mvControlling->Ft ) < 50000.0 ) {
                    IncSpeed();
                }
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( std::abs( mvControlling->Ft ) > 30.0 ) || ( ( OrderCurrentGet() & Disconnect ) == 0 ) ); } );
            break;
        }

        case driver_hint::secondcontrollersetzero: {
            if( AIControllFlag ) {
                mvControlling->DecScndCtrl( 2 );
            }
            remove_master_controller_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvControlling->IsScndCtrlNoPowerPos() ); } );
            break;
        }

        case driver_hint::waterpumpon: {
            if( AIControllFlag ) {
                mvControlling->WaterPumpSwitch( true );
            }
            remove_hint( driver_hint::waterpumpoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterPump };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == true ) && ( device.is_disabled == false ) ) || ( device.is_active == true ) ); } );
            break;
        }
        case driver_hint::waterpumpoff: {
            if( AIControllFlag ) {
                mvControlling->WaterPumpSwitch( false );
            }
            remove_hint( driver_hint::waterpumpon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterPump };
                    return ( ( device.start_type != start_t::manual ) || ( ( device.is_enabled == false ) && ( device.is_active == false ) ) ); } );
            break;
        }
        case driver_hint::waterpumpbreakeron: {
            if( AIControllFlag ) {
                mvControlling->WaterPumpBreakerSwitch( true );
            }
            remove_hint( driver_hint::waterpumpbreakeroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterPump };
                    return ( device.breaker == true ); } );
            break;
        }
        case driver_hint::waterpumpbreakeroff: {
            if( AIControllFlag ) {
                mvControlling->WaterPumpBreakerSwitch( false );
            }
            remove_hint( driver_hint::waterpumpbreakeron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterPump };
                    return ( device.breaker == false ); } );
            break;
        }
        case driver_hint::waterheateron: {
            if( AIControllFlag ) {
                mvControlling->WaterHeaterSwitch( true );
            }
            remove_hint( driver_hint::waterheateroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterHeater };
                    return ( device.is_enabled == true ); } );
            break;
        }
        case driver_hint::waterheateroff: {
            if( AIControllFlag ) {
                mvControlling->WaterHeaterSwitch( false );
            }
            remove_hint( driver_hint::waterheateron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterHeater };
                    return ( device.is_enabled == false ); } );
            break;
        }
        case driver_hint::waterheaterbreakeron: {
            if( AIControllFlag ) {
                mvControlling->WaterHeaterBreakerSwitch( true );
            }
            remove_hint( driver_hint::waterheaterbreakeroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterHeater };
                    return ( device.breaker == true ); } );
            break;
        }
        case driver_hint::waterheaterbreakeroff: {
            if( AIControllFlag ) {
                mvControlling->WaterHeaterBreakerSwitch( false );
            }
            remove_hint( driver_hint::waterheaterbreakeron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvControlling->WaterHeater };
                    return ( device.breaker == false ); } );
            break;
        }
        case driver_hint::watercircuitslinkon: {
            if( AIControllFlag ) {
                mvControlling->WaterCircuitsLinkSwitch( true );
            }
            remove_hint( driver_hint::watercircuitslinkoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->dizel_heat.auxiliary_water_circuit == false ) || ( mvControlling->WaterCircuitsLink == true ) ); } );
            break;
        }
        case driver_hint::watercircuitslinkoff: {
            if( AIControllFlag ) {
                mvControlling->WaterCircuitsLinkSwitch( false );
            }
            remove_hint( driver_hint::watercircuitslinkon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->dizel_heat.auxiliary_water_circuit == false ) || ( mvControlling->WaterCircuitsLink == false ) ); } );
            break;
        }
        case driver_hint::waittemperaturetoolow: {
            // NOTE: this action doesn't have AI component
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( false == IsHeatingTemperatureTooLow ); } );
            break;
        }
        case driver_hint::waitpressuretoolow: {
            // NOTE: this action doesn't have AI component
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->ScndPipePress > 4.5 ) || ( mvControlling->VeselVolume == 0.0 ) ); } );
            break;
        }
        case driver_hint::waitpantographpressuretoolow: {
            // NOTE: this action doesn't have AI component
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvPantographUnit->PantPress >= ( is_emu() ? ( mvPantographUnit->PantPressLockActive ? 4.6 : 2.6 ) : 4.2 ) ); } );
            break;
        }
        case driver_hint::waitloadexchange: {
            // NOTE: this action doesn't have AI component
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ExchangeTime <= 0 ); } );
            break;
        }
        case driver_hint::waitdeparturetime: {
            // NOTE: this action doesn't have AI component
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAtPassengerStop == false ); } );
            break;
        }

        // train brake and braking force. mutually exclusive:
        // driver_hint_trainbrakesetpipeunlock
        // driver_hint_trainbrakerelease
        // driver_hint_brakingforcedecrease
        // driver_hint_brakingforceincrease
        // driver_hint_brakingforcesetzero
        // driver_hint_brakingforcelap
        case driver_hint::trainbrakesetpipeunlock: {
            if( AIControllFlag ) {
                if( mvOccupied->HandleUnlock != -3 ) {
                    while( ( BrakeCtrlPosition >= mvOccupied->HandleUnlock )
                        && ( BrakeLevelAdd( -1 ) ) ) {
                        // all work is done in the header
                        ;
                    }
                }
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->HandleUnlock == -3 ) || ( BrakeCtrlPosition == mvOccupied->HandleUnlock ) ); } );
            break;
        }
        case driver_hint::trainbrakerelease: {
            if( AIControllFlag ) {
//                mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_RP ) );
                BrakeLevelSet( gbh_RP );
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( is_equal(  mvOccupied->fBrakeCtrlPos, mvOccupied->Handle->GetPos( bh_RP ), 0.2 )
							|| ( mvOccupied->Handle->Time && ( mvOccupied->Handle->GetCP() > mvOccupied->HighPipePress - 0.05) ) ); } );
//                    return ( BrakeCtrlPosition == gbh_RP ); } );
            break;
        }
        case driver_hint::trainbrakeapply: {
            if( AIControllFlag ) {
                // za radą yB ustawiamy pozycję 3 kranu (ruszanie kranem w innych miejscach powino zostać wyłączone)
                if( ( BrakeSystem == TBrakeSystem::ElectroPneumatic ) && ( false == ForcePNBrake ) ) {
                    mvOccupied->BrakeLevelSet( mvOccupied->Handle->GetPos( bh_EPB ) );
                }
                else {
                    BrakeCtrlPosition = 3;
                }
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsConsistBraked /*|| ( ( OrderCurrentGet() & Disconnect ) == 0 ) */ ); } );
            break;
        }
        case driver_hint::brakingforcedecrease: {
            if( AIControllFlag ) {
                auto const brakingcontrolschange { DecBrake() };
                // set optional delay between brake adjustments
                if( ( brakingcontrolschange ) && ( Actionparameter > 0 ) ) {
                    fBrakeTime = Actionparameter;
                }
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( std::abs( mvControlling->Fb ) <= Parameter ); },
                std::min( 0.0, 0.95 * std::abs( mvControlling->Fb ) ) ); // keep hint until 5% decrease
            break;
        }
        case driver_hint::brakingforceincrease: {
            if( AIControllFlag ) {
                auto const brakingcontrolschange { IncBrake() };
                // set optional delay between brake adjustments
                if( ( brakingcontrolschange ) && ( Actionparameter > 0 ) ) {
                    fBrakeTime = Actionparameter;
                }
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( std::abs( mvControlling->Fb ) > Parameter ) || ( is_equal( mvOccupied->fBrakeCtrlPos, mvOccupied->Handle->GetPos( bh_EB ), 0.2 ) ) ); },
                std::max(
                    0.0,
                    std::max(
                        std::abs( mvControlling->Fb ) * 1.05,
                        std::abs( mvControlling->Fb ) + 5.0 ) ) );
            break;
        }
        case driver_hint::brakingforcesetzero: { // releases both train and independent brake
            if( AIControllFlag ) {
                while( true == DecBrake() ) { ; }
            }
            remove_train_brake_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( Ready ) && ( fReady < 0.4 ) && ( is_equal( mvOccupied->fBrakeCtrlPos, mvOccupied->Handle->GetPos( bh_RP ), 0.2 ) ) ); } );
            break;
        }
        case driver_hint::brakingforcelap: {
            if( AIControllFlag ) {
                LapBrake();
            }
            remove_train_brake_hints();
            // NOTE: this action doesn't have human component
            // TBD, TODO: provide one?
            break;
        }
        // independent brake
        case driver_hint::independentbrakeapply: {
            if( AIControllFlag ) {
                if( mvOccupied->LocalBrakePosA < 1.0 ) {
                    mvOccupied->IncLocalBrakeLevel( LocalBrakePosNo );
                    if (mvOccupied->EIMCtrlEmergency) {
                        mvOccupied->DecLocalBrakeLevel(1);
					}
                }
            }
            remove_hint( driver_hint::independentbrakerelease );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->LocalBrakePosA >= Parameter ); },
                ( LocalBrakePosNo - ( mvOccupied->EIMCtrlEmergency ? 1 : 0 ) ) / LocalBrakePosNo );
            break;
        }
        case driver_hint::independentbrakerelease: {
            if( AIControllFlag ) {
                ZeroLocalBrake();
            }
            remove_hint( driver_hint::independentbrakeapply );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->LocalBrakePosA < 0.05 ); } );
            break;
        }
        // reverser
        case driver_hint::directionforward: {
            if( AIControllFlag ) {
                OrderDirectionChange( 1, mvOccupied );
            }
            remove_reverser_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->DirActive * mvOccupied->CabActive > 0 ); } );
            break;
        }
        case driver_hint::directionbackward: {
            if( AIControllFlag ) {
                OrderDirectionChange( -1, mvOccupied );
            }
            remove_reverser_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->DirActive * mvOccupied->CabActive < 0 ); } );
            break;
        }
        case driver_hint::directionother: {
            if( AIControllFlag ) {
//                DirectionForward( mvOccupied->DirAbsolute < 0 );
                OrderDirectionChange( iDirectionOrder, mvOccupied );
                DirectionChange();
            }
            remove_reverser_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( iDirection == iDirectionOrder ); } );
            break;
        }
        case driver_hint::directionnone: {
            if( AIControllFlag ) {
                ZeroDirection();
            }
            remove_reverser_hints();
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->DirActive == 0 ); } );
            break;
        }
        // anti-slip systems
        case driver_hint::sandingon: {
            if( AIControllFlag ) {
                mvControlling->Sandbox( true );
            }
            remove_hint( driver_hint::sandingoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->Sand == 0 ) || ( mvControlling->SandDose == true ) || ( mvControlling->SlippingWheels == false ) ); } );
            break;
        }
        case driver_hint::sandingoff: {
            if( AIControllFlag ) {
                mvControlling->Sandbox( false );
            }
            remove_hint( driver_hint::sandingon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->Sand == 0 ) || ( mvControlling->SandDose == false ) ); } );
            break;
        }
        case driver_hint::antislip: {
            if( AIControllFlag ) {
                mvControlling->AntiSlippingButton();
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvControlling->ASBType != 1 ) || ( ( mvControlling->Hamulec->GetBrakeStatus() & b_asb ) != 0 ) || ( mvControlling->SlippingWheels == false ) ); } );
            break;
        }
        // horns
        case driver_hint::hornon: {
            if( AIControllFlag ) {
                mvOccupied->WarningSignal = static_cast<int>( Actionparameter );
            }
            remove_hint( driver_hint::hornoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    // NOTE: we provide slightly larger horn activation window for human driver
                    return ( ( fWarningDuration + 5.0 < 0.05 ) || ( mvOccupied->WarningSignal != 0 ) ); } );
            break;
        }
        case driver_hint::hornoff: {
            if( AIControllFlag ) {
                mvOccupied->WarningSignal = 0;
            }
/*
            // NOTE: for human driver the hint to switch horn on will disappear automatically
            remove_hint( driver_hint::hornon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->WarningSignal == 0 ); } );
*/
            break;
        }
        // door locks
        case driver_hint::consistdoorlockson: {
            if( AIControllFlag ) {
                mvOccupied->LockDoors( true );
            }
            // TODO: remove other door lock hints
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->Doors.has_lock == false ) || ( mvOccupied->Doors.lock_enabled == true ) ); } );
            break;
        }
        // departure signal
        case driver_hint::departuresignalon: {
            if( AIControllFlag ) {
                mvOccupied->signal_departure( true );
            }
            remove_hint( driver_hint::departuresignaloff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->Doors.has_warning == false ) || ( mvOccupied->Doors.has_autowarning == true ) || ( mvOccupied->DepartureSignal == true ) || mvOccupied->Vel > 5.0 ); } );
            break;
        }
        case driver_hint::departuresignaloff: {
            if( AIControllFlag ) {
                mvOccupied->signal_departure( false );
            }
            remove_hint( driver_hint::departuresignalon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( ( mvOccupied->Doors.has_warning == false ) || ( mvOccupied->Doors.has_autowarning == true ) || ( mvOccupied->DepartureSignal == false ) ); } );
            break;
        }
        // consist doors
        case driver_hint::doorrightopen: {
            if( ( AIControllFlag )
             || ( pVehicle->MoverParameters->Doors.open_control == control_t::conductor ) ) {
                pVehicle->MoverParameters->OperateDoors( side::right, true );
            }
            remove_hint( driver_hint::doorrightclose );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorOpen[ side::right ] == true ); } );
            break;
        }
        case driver_hint::doorrightclose: {
            if( ( AIControllFlag )
             || ( pVehicle->MoverParameters->Doors.open_control == control_t::conductor ) ) {
                pVehicle->MoverParameters->OperateDoors( side::right, false );
            }
            remove_hint( driver_hint::doorrightopen );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorOpen[ side::right ] == false ); } );
            break;
        }
        case driver_hint::doorleftopen: {
            if( ( AIControllFlag )
             || ( pVehicle->MoverParameters->Doors.open_control == control_t::conductor ) ) {
                pVehicle->MoverParameters->OperateDoors( side::left, true );
            }
            remove_hint( driver_hint::doorleftclose );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorOpen[ side::left ] == true ); } );
            break;
        }
        case driver_hint::doorleftclose: {
            if( ( AIControllFlag )
             || ( pVehicle->MoverParameters->Doors.open_control == control_t::conductor ) ) {
                pVehicle->MoverParameters->OperateDoors( side::left, false );
            }
            remove_hint( driver_hint::doorleftopen );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorOpen[ side::left ] == false ); } );
            break;
        }
        case driver_hint::doorrightpermiton: {
            if( AIControllFlag ) {
                pVehicle->MoverParameters->PermitDoors( side::right, true );
            }
            remove_hint( driver_hint::doorrightpermitoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorPermitActive[ side::right ] == true ); } );
            break;
        }
        case driver_hint::doorrightpermitoff: {
            if( AIControllFlag ) {
                pVehicle->MoverParameters->PermitDoors( side::right, false );
            }
            remove_hint( driver_hint::doorrightpermiton );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorPermitActive[ side::right ] == false ); } );
            break;
        }
        case driver_hint::doorleftpermiton: {
            if( AIControllFlag ) {
                pVehicle->MoverParameters->PermitDoors( side::left, true );
            }
            remove_hint( driver_hint::doorleftpermitoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorPermitActive[ side::left ] == true ); } );
            break;
        }
        case driver_hint::doorleftpermitoff: {
            if( AIControllFlag ) {
                pVehicle->MoverParameters->PermitDoors( side::left, false );
            }
            remove_hint( driver_hint::doorleftpermiton );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( IsAnyDoorPermitActive[ side::left ] == false ); } );
            break;
        }
        // consist lights
        case driver_hint::consistlightson: {
            if( AIControllFlag ) {
                mvOccupied->CompartmentLightsSwitch( true );
                mvOccupied->CompartmentLightsSwitchOff( false );
            }
            remove_hint( driver_hint::consistlightsoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->CompartmentLights };
                    return ( ( Global.fLuminance * ConsistShade > 0.40 ) || ( device.start_type != start_t::manual ) || ( ( device.is_enabled == true ) && ( device.is_disabled == false ) ) || ( device.is_active == true ) ); } );
            break;
        }
        case driver_hint::consistlightsoff: {
            if( AIControllFlag ) {
                mvOccupied->CompartmentLightsSwitch( false );
                mvOccupied->CompartmentLightsSwitchOff( true );
            }
            remove_hint( driver_hint::consistlightson );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { mvOccupied->CompartmentLights };
                    return ( ( Global.fLuminance * ConsistShade < 0.35 ) || ( device.start_type != start_t::manual ) || ( ( device.is_enabled == false ) && ( device.is_active == false ) ) ); } );
            break;
        }
        // consist heating
        case driver_hint::consistheatingon: {
            if( AIControllFlag ) {
                mvOccupied->HeatingSwitch( true );
            }
            remove_hint( driver_hint::consistheatingoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->HeatingAllow == true ); } );
            break;
        }
        case driver_hint::consistheatingoff: {
            if( AIControllFlag ) {
                mvOccupied->HeatingSwitch( false );
            }
            remove_hint( driver_hint::consistheatingon );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( mvOccupied->HeatingAllow == false ); } );
            break;
        }

        case driver_hint::securitysystemreset: {
            if( AIControllFlag ) {
                if (mvOccupied->SecuritySystem.is_blinking())
                    mvOccupied->SecuritySystemReset();
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return !mvOccupied->SecuritySystem.is_vigilance_blinking(); } );
            break;
        }
        case driver_hint::shpsystemreset: {
            if( AIControllFlag ) {
                if (mvOccupied->SecuritySystem.is_cabsignal_blinking())
                    mvOccupied->SecuritySystem.cabsignal_reset();
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return !mvOccupied->SecuritySystem.is_cabsignal_blinking(); } );
            break;
        }
        case driver_hint::couplingadapterattach: {
            // TODO: run also for potential settings-based virtual assistant
            if( AIControllFlag ) {
                pVehicles[ end::front ]->attach_coupler_adapter( Actionparameter );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { pVehicles[ end::front ]->MoverParameters->Couplers[ static_cast<int>( Parameter ) ] };
                    return ( device.type() == TCouplerType::Automatic ); } );
            break;
        }
        case driver_hint::couplingadapterremove: {
            if( AIControllFlag || Global.AITrainman ) {
                pVehicles[ end::front ]->remove_coupler_adapter( Actionparameter );
            }
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const &device { pVehicles[ end::front ]->MoverParameters->Couplers[ static_cast<int>( Parameter ) ] };
                    return ( false == device.has_adapter() ); } );
            break;
        }
        // lights
        case driver_hint::headcodepc1: {
            if( AIControllFlag ) {
                pVehicles[ end::front ]->RaLightsSet( light::headlight_left | light::headlight_right | light::headlight_upper, -1 );
            }
            remove_hint( driver_hint::headcodepc2 );
            remove_hint( driver_hint::headcodetb1 );
            remove_hint( driver_hint::lightsoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( pVehicles[ end::front ]->has_signal_pc1_on() ); } );
            break;
        }
        case driver_hint::headcodepc2: {
            if( AIControllFlag ) {
                pVehicles[ end::front ]->RaLightsSet( light::redmarker_left | light::headlight_right | light::headlight_upper, -1 );
            }
            remove_hint( driver_hint::headcodepc1 );
            remove_hint( driver_hint::headcodetb1 );
            remove_hint( driver_hint::lightsoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( pVehicles[ end::front ]->has_signal_pc2_on() ); } );
            break;
        }
        case driver_hint::headcodetb1: {
            // HACK: the 'front' and 'rear' of the consist is determined by current consist direction
            // since direction shouldn't affect Tb1 light configuration, we 'counter' this behaviour by virtually swapping end vehicles
            if( AIControllFlag ) {
                if( mvOccupied->DirActive >= 0 ) { Lights( light::headlight_right, light::headlight_left ); }
                else                             { Lights( light::headlight_left, light::headlight_right ); }
            }
            remove_hint( driver_hint::headcodepc1 );
            remove_hint( driver_hint::headcodepc2 );
            remove_hint( driver_hint::headcodepc5 );
            remove_hint( driver_hint::lightsoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const activeend { mvOccupied->CabActive >= 0 ? end::front : end::rear };
                    auto const consistfront { mvOccupied->DirActive >= 0 ? end::front : end::rear };
                    return (
                        pVehicles[ consistfront ]->has_signal_on( activeend,  light::headlight_right )
                     && pVehicles[ 1 - consistfront ]->has_signal_on( 1 - activeend, light::headlight_left ) ); } );
            break;
        }
        case driver_hint::headcodepc5: {
            if( ( AIControllFlag )
             || ( Global.AITrainman && ( false == pVehicles[ end::rear ]->is_connected( pVehicle, coupling::control ) ) ) ) {
                pVehicles[ end::rear ]->RaLightsSet( -1, light::redmarker_left | light::redmarker_right | light::rearendsignals );
            }
            remove_hint( driver_hint::headcodetb1 );
            remove_hint( driver_hint::lightsoff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( pVehicles[ end::rear ]->has_signal_pc5_on() ); } );
            break;
        }
        case driver_hint::lightsoff: {
            if( AIControllFlag ) {
                Lights( 0, 0 );
            }
            remove_hint( driver_hint::headcodepc1 );
            remove_hint( driver_hint::headcodepc2 );
            remove_hint( driver_hint::headcodepc5 );
            remove_hint( driver_hint::headcodetb1 );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    auto const activeend { mvOccupied->CabActive >= 0 ? end::front : end::rear };
                    auto const consistfront { mvOccupied->DirActive >= 0 ? end::front : end::rear };
                    return (
                        pVehicles[ consistfront ]->has_signal_on( activeend,  0 )
                     && pVehicles[ 1 - consistfront ]->has_signal_on( 1 - activeend, 0 ) ); } );
            break;
        }
        // releaser
        case driver_hint::releaseron: {
            if( AIControllFlag ) {
                mvOccupied->BrakeReleaser( 1 );
            }
            remove_hint( driver_hint::releaseroff );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( true == mvOccupied->Hamulec->Releaser() ); } );
            break;
        }
        case driver_hint::releaseroff: {
            if( AIControllFlag ) {
                mvOccupied->BrakeReleaser( 0 );
            }
            remove_hint( driver_hint::releaseron );
            hint(
                Action,
                [this](float const Parameter) -> bool {
                    return ( false == mvOccupied->Hamulec->Releaser() ); } );
            break;
        }

        default: {
            break;
        }
    }
}
