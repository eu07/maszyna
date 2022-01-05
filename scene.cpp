/*
This Source Code Form is subject to the
terms of the Mozilla Public License, v.
2.0. If a copy of the MPL was not
distributed with this file, You can
obtain one at
http://mozilla.org/MPL/2.0/.
*/

#include "stdafx.h"
#include "scene.h"

#include "simulation.h"
#include "Globals.h"
#include "Camera.h"
#include "AnimModel.h"
#include "Event.h"
#include "EvLaunch.h"
#include "Timer.h"
#include "Logs.h"
#include "sn_utils.h"
#include "renderer.h"
#include "widgets/map_objects.h"

namespace scene {

std::string const EU07_FILEEXTENSION_REGION { ".sbt" };
std::uint32_t const EU07_FILEHEADER { MAKE_ID4( 'E','U','0','7' ) };
std::uint32_t const EU07_FILEVERSION_REGION { MAKE_ID4( 'S', 'B', 'T', 1 ) };

// potentially activates event handler with the same name as provided node, and within handler activation range
void
basic_cell::on_click( TAnimModel const *Instance ) {

    for( auto *launcher : m_eventlaunchers ) {
        if( ( launcher->name() == Instance->name() )
         && ( glm::length2( launcher->location() - Instance->location() ) < launcher->dRadius )
         && ( true == launcher->check_conditions() ) ) {
            launch_event( launcher, true );
        }
    }
}

// legacy method, finds and assigns traction piece to specified pantograph of provided vehicle
void
basic_cell::update_traction( TDynamicObject *Vehicle, int const Pantographindex ) {
    // Winger 170204 - szukanie trakcji nad pantografami
    auto const vFront = glm::make_vec3( Vehicle->VectorFront().getArray() ); // wektor normalny dla płaszczyzny ruchu pantografu
    auto const vUp = glm::make_vec3( Vehicle->VectorUp().getArray() ); // wektor pionu pudła (pochylony od pionu na przechyłce)
    auto const vLeft = glm::make_vec3( Vehicle->VectorLeft().getArray() ); // wektor odległości w bok (odchylony od poziomu na przechyłce)
    auto const position = glm::dvec3 { Vehicle->GetPosition() }; // współrzędne środka pojazdu

    auto pantograph = Vehicle->pants[ Pantographindex ].fParamPants;
    auto const pantographposition = position + ( vLeft * pantograph->vPos.z ) + ( vUp * pantograph->vPos.y ) + ( vFront * pantograph->vPos.x );
    
    for( auto *traction : m_directories.traction ) {

        // współczynniki równania parametrycznego
        auto const paramfrontdot = glm::dot( traction->vParametric, vFront );
        auto const fRaParam =
            -( glm::dot( traction->pPoint1, vFront ) - glm::dot( pantographposition, vFront ) )
            / ( paramfrontdot != 0.0 ?
                    paramfrontdot :
                    0.001 ); // div0 trap

        if( ( fRaParam < -0.001 )
         || ( fRaParam >  1.001 ) ) { continue; }
        // jeśli tylko jest w przedziale, wyznaczyć odległość wzdłuż wektorów vUp i vLeft
        // punkt styku płaszczyzny z drutem (dla generatora łuku el.)
        auto const vStyk = traction->pPoint1 + fRaParam * traction->vParametric;
        // wektor musi się mieścić w przedziale ruchu pantografu
        auto const vGdzie = vStyk - pantographposition;
        auto fVertical = glm::dot( vGdzie, vUp );
        if( fVertical >= 0.0 ) {
            // jeśli ponad pantografem (bo może łapać druty spod wiaduktu)
            auto const fHorizontal = std::abs( glm::dot( vGdzie, vLeft ) ) - pantograph->fWidth;

            if( ( Global.bEnableTraction )
             && ( fVertical < pantograph->PantWys - 0.15 ) ) {
                // jeśli drut jest niżej niż 15cm pod ślizgiem przełączamy w tryb połamania, o ile jedzie;
                // (bEnableTraction) aby dało się jeździć na koślawych sceneriach
                // i do tego jeszcze wejdzie pod ślizg
                if( fHorizontal <= 0.0 ) {
                    // 0.635 dla AKP-1 AKP-4E
                    SetFlag( Vehicle->MoverParameters->DamageFlag, dtrain_pantograph );
                    pantograph->PantWys = -1.0; // ujemna liczba oznacza połamanie
                    pantograph->hvPowerWire = nullptr; // bo inaczej się zasila w nieskończoność z połamanego
                    if( Vehicle->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo > 0 ) {
                        // liczba pantografów teraz będzie mniejsza
                        --Vehicle->MoverParameters->EnginePowerSource.CollectorParameters.CollectorsNo;
                    }
                    ErrorLog( "Bad traction: " + Vehicle->name() + " broke pantograph at " + to_string( pantographposition ), logtype::traction );

                }
            }
            else if( fVertical < pantograph->PantTraction ) {
                // ale niżej, niż poprzednio znaleziony
                if( fHorizontal <= 0.0 ) {
                    // 0.635 dla AKP-1 AKP-4E
                    // to się musi mieścić w przedziale zaleznym od szerokości pantografu
                    pantograph->hvPowerWire = traction; // jakiś znaleziony
                    pantograph->PantTraction = fVertical; // zapamiętanie nowej wysokości
                }
                else if( fHorizontal < pantograph->fWidthExtra ) {
                    // czy zmieścił się w zakresie nabieżnika? problem jest, gdy nowy drut jest wyżej,
                    // wtedy pantograf odłącza się od starego, a na podniesienie do nowego potrzebuje czasu
                    // korekta wysokości o nabieżnik - drut nad nabieżnikiem jest geometrycznie jakby nieco wyżej
                    fVertical += 0.15 * fHorizontal / pantograph->fWidthExtra;
                    if( fVertical < pantograph->PantTraction ) {
                        // gdy po korekcie jest niżej, niż poprzednio znaleziony
                        // gdyby to wystarczyło, to możemy go uznać
                        pantograph->hvPowerWire = traction; // może być
                        pantograph->PantTraction = fVertical; // na razie liniowo na nabieżniku, dokładność poprawi się później
                    }
                }
            }
        }
    }
}

// legacy method, updates sounds and polls event launchers within radius around specified point
void
basic_cell::update_events() {

    // event launchers
    for( auto *launcher : m_eventlaunchers ) {
        glm::dvec3 campos = Global.pCamera.Pos;
        double radius = launcher->dRadius;
        if (launcher->train_triggered && simulation::Train) {
            campos = simulation::Train->Dynamic()->HeadPosition();
            radius *= Timer::GetDeltaTime() * simulation::Train->Dynamic()->GetVelocity() * 0.277;
        }

        if( launcher->check_conditions()
            && ( radius < 0.0
                || glm::distance2( launcher->location(), campos ) < launcher->dRadius ) ) {
            if( launcher->check_activation() )
                launch_event( launcher, true );
            if( launcher->check_activation_key() )
                launch_event( launcher, true );
        }
    }
}

// legacy method, updates sounds and polls event launchers within radius around specified point
void
basic_cell::update_sounds() {

    for( auto *sound : m_sounds ) {
        sound->play_event();
    }
    // TBD, TODO: move to sound renderer
    for( auto *path : m_paths ) {
        // dźwięki pojazdów, również niewidocznych
        path->RenderDynSounds();
    }
}

// legacy method, triggers radio-stop procedure for all vehicles located on paths in the cell
void
basic_cell::radio_stop() {

    for( auto *path : m_paths ) {
        path->RadioStop();
    }
}

// legacy method, adds specified path to the list of pieces undergoing state change
bool
basic_cell::RaTrackAnimAdd( TTrack *Track ) {

    if( false == m_geometrycreated ) {
        // nie ma animacji, gdy nie widać
        return true;
    }
    if (tTrackAnim)
        tTrackAnim->RaAnimListAdd(Track);
    else
        tTrackAnim = Track;
    return false; // będzie animowane...
}

// legacy method, updates geometry for pieces in the animation list
void
basic_cell::RaAnimate( unsigned int const Framestamp ) {

    if( ( tTrackAnim == nullptr )
     || ( Framestamp == m_framestamp ) ) {
        // nie ma nic do animowania
        return;
    }
    tTrackAnim = tTrackAnim->RaAnimate(); // przeliczenie animacji kolejnego

    m_framestamp = Framestamp;
}

// sends content of the class to provided stream
void
basic_cell::serialize( std::ostream &Output ) const {

    // region file version 0, cell data
    // bounding area
    m_area.serialize( Output );
    // NOTE: cell activation flag is set dynamically on load
    // cell shapes
    // shape count followed by opaque shape data
    sn_utils::ls_uint32( Output, m_shapesopaque.size() );
    for( auto const &shape : m_shapesopaque ) {
        shape.serialize( Output );
    }
    // shape count followed by translucent shape data
    sn_utils::ls_uint32( Output, m_shapestranslucent.size() );
    for( auto const &shape : m_shapestranslucent ) {
        shape.serialize( Output );
    }
    // cell lines
    // line count followed by lines data
    sn_utils::ls_uint32( Output, m_lines.size() );
    for( auto const &lines : m_lines ) {
        lines.serialize( Output );
    }
}

// restores content of the class from provided stream
void
basic_cell::deserialize( std::istream &Input ) {

    // region file version 0, cell data
    // bounding area
    m_area.deserialize( Input );
    // cell shapes
    // shape count followed by opaque shape data
    auto itemcount { sn_utils::ld_uint32( Input ) };
    while( itemcount-- ) {
        m_shapesopaque.emplace_back( shape_node().deserialize( Input ) );
    }
    itemcount = sn_utils::ld_uint32( Input );
    while( itemcount-- ) {
        m_shapestranslucent.emplace_back( shape_node().deserialize( Input ) );
    }
    itemcount = sn_utils::ld_uint32( Input );
    while( itemcount-- ) {
        m_lines.emplace_back( lines_node().deserialize( Input ) );
    }
    // cell activation flag
    m_active = (
        ( true == m_active )
     || ( false == m_shapesopaque.empty() )
     || ( false == m_shapestranslucent.empty() )
     || ( false == m_lines.empty() ) );
}

// sends content of the class in legacy (text) format to provided stream
void
basic_cell::export_as_text( std::ostream &Output ) const {

    // text format export dumps only relevant basic objects
    // sounds
    for( auto const *sound : m_sounds ) {
        sound->export_as_text( Output );
    }
}

// adds provided shape to the cell
void
basic_cell::insert( shape_node Shape ) {

    m_active = true;

    // re-calculate cell radius, in case shape geometry extends outside the cell's boundaries
    m_area.radius = std::max<float>(
        m_area.radius,
	    glm::length( m_area.center - Shape.data().area.center ) + Shape.radius() );

    auto const &shapedata { Shape.data() };
    auto &shapes = (
        shapedata.translucent ?
            m_shapestranslucent :
            m_shapesopaque );
    for( auto &targetshape : shapes ) {
        // try to merge shapes with matching view ranges...
        auto const &targetshapedata { targetshape.data() };
        if( ( shapedata.rangesquared_min == targetshapedata.rangesquared_min )
         && ( shapedata.rangesquared_max == targetshapedata.rangesquared_max )
        // ...and located close to each other (within arbitrary limit of 25m)
         && ( glm::length( shapedata.area.center - targetshapedata.area.center ) < 25.0 ) ) {

            if( true == targetshape.merge( Shape ) ) {
                // if the shape was merged there's nothing left to do
                return;
            }
        }
    }
    // otherwise add the shape to the relevant list
    Shape.origin( m_area.center );
    shapes.emplace_back( Shape );
}

// adds provided lines to the cell
void
basic_cell::insert( lines_node Lines ) {

    m_active = true;

    auto const &linesdata { Lines.data() };
    for( auto &targetlines : m_lines ) {
        // try to merge shapes with matching view ranges...
        auto const &targetlinesdata { targetlines.data() };
        if( ( linesdata.rangesquared_min == targetlinesdata.rangesquared_min )
         && ( linesdata.rangesquared_max == targetlinesdata.rangesquared_max )
        // ...and located close to each other (within arbitrary limit of 10m)
         && ( glm::length( linesdata.area.center - targetlinesdata.area.center ) < 10.0 ) ) {

            if( true == targetlines.merge( Lines ) ) {
                // if the shape was merged there's nothing left to do
                return;
            }
        }
    }
    // otherwise add the shape to the relevant list
    Lines.origin( m_area.center );
    m_lines.emplace_back( Lines );
}

// adds provided path to the cell
void
basic_cell::insert( TTrack *Path ) {

    m_active = true;

    Path->origin( m_area.center );
    m_paths.emplace_back( Path );
    // animation hook
    Path->RaOwnerSet( this );
    // re-calculate cell radius, in case track extends outside the cell's boundaries
    m_area.radius = std::max(
        m_area.radius,
        static_cast<float>( glm::length( m_area.center - Path->location() ) + Path->radius() + 25.f ) ); // extra margin to prevent driven vehicle from flicking
}

// adds provided traction piece to the cell
void
basic_cell::insert( TTraction *Traction ) {

    m_active = true;

    Traction->origin( m_area.center );
    m_traction.emplace_back( Traction );
    // re-calculate cell bounding area, in case traction piece extends outside the cell's boundaries
    enclose_area( Traction );
}

// adds provided model instance to the cell
void
basic_cell::insert( TAnimModel *Instance ) {

    m_active = true;

    auto const flags = Instance->Flags();
    auto alpha =
        ( Instance->Material() != nullptr ?
            Instance->Material()->textures_alpha :
            0x30300030 );

    // assign model to appropriate render phases
    if( alpha & flags & 0x2F2F002F ) {
        // translucent pieces
        m_instancetranslucent.emplace_back( Instance );
    }
    alpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if( alpha & flags & 0x1F1F001F ) {
        // opaque pieces
        m_instancesopaque.emplace_back( Instance );
    }
   // re-calculate cell bounding area, in case model extends outside the cell's boundaries
    enclose_area( Instance );
}

// adds provided sound instance to the cell
void
basic_cell::insert( sound_source *Sound ) {

    m_active = true;

    m_sounds.emplace_back( Sound );
    // NOTE: sound sources are virtual 'points' hence they don't ever expand cell range
}

// adds provided sound instance to the cell
void
basic_cell::insert( TEventLauncher *Launcher ) {

    m_active = true;

    m_eventlaunchers.emplace_back( Launcher );
    // re-calculate cell bounding area, in case launcher range extends outside the cell's boundaries
    enclose_area( Launcher );
}

// adds provided memory cell to the cell
void
basic_cell::insert( TMemCell *Memorycell ) {

    m_active = true;

    m_memorycells.emplace_back( Memorycell );
    // NOTE: memory cells are virtual 'points' hence they don't ever expand cell range
}

// removes provided model instance from the cell
void
basic_cell::erase( TAnimModel *Instance ) {

    auto const flags = Instance->Flags();
    auto alpha =
        ( Instance->Material() != nullptr ?
            Instance->Material()->textures_alpha :
            0x30300030 );

    if( alpha & flags & 0x2F2F002F ) {
        // instance has translucent pieces
        m_instancetranslucent.erase(
            std::remove_if(
                std::begin( m_instancetranslucent ), std::end( m_instancetranslucent ),
                [=]( TAnimModel *instance ) {
                    return instance == Instance; } ),
            std::end( m_instancetranslucent ) );
    }
    alpha ^= 0x0F0F000F; // odwrócenie flag tekstur, aby wyłapać nieprzezroczyste
    if( alpha & flags & 0x1F1F001F ) {
        // instance has opaque pieces
        m_instancesopaque.erase(
            std::remove_if(
                std::begin( m_instancesopaque ), std::end( m_instancesopaque ),
                [=]( TAnimModel *instance ) {
                    return instance == Instance; } ),
            std::end( m_instancesopaque ) );
    }
    // TODO: update cell bounding area
}

// removes provided memory cell from the cell
void
basic_cell::erase( TMemCell *Memorycell ) {

    m_memorycells.erase(
        std::remove_if(
            std::begin( m_memorycells ), std::end( m_memorycells ),
            [=]( TMemCell *memorycell ) {
                return memorycell == Memorycell; } ),
        std::end( m_memorycells ) );
}

// registers provided path in the lookup directory of the cell
void
basic_cell::register_end( TTrack *Path ) {

    m_directories.paths.emplace_back( Path );
    // eliminate potential duplicates
    m_directories.paths.erase(
        std::unique(
            std::begin( m_directories.paths ),
            std::end( m_directories.paths ) ),
        std::end( m_directories.paths ) );
}

// registers provided traction piece in the lookup directory of the cell
void
basic_cell::register_end( TTraction *Traction ) {

    m_directories.traction.emplace_back( Traction );
    // eliminate potential duplicates
    m_directories.traction.erase(
        std::unique(
            std::begin( m_directories.traction ),
            std::end( m_directories.traction ) ),
        std::end( m_directories.traction ) );
}

// find a vehicle located nearest to specified point, within specified radius, optionally ignoring vehicles without drivers. reurns: located vehicle and distance
std::tuple<TDynamicObject *, float>
basic_cell::find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler ) const {

    TDynamicObject *vehiclenearest { nullptr };
    float leastdistance { std::numeric_limits<float>::max() };
    float distance;
    float const distancecutoff { Radius * Radius }; // we'll ignore vehicles farther than this

    for( auto *path : m_paths ) {
        for( auto *vehicle : path->Dynamics ) {
            if( ( true == Onlycontrolled )
             && ( vehicle->Mechanik == nullptr ) ) {
                continue;
            }
            if( false == Findbycoupler ) {
                // basic search, checks vehicles' center points
                distance = glm::length2( glm::dvec3{ vehicle->GetPosition() } - Point );
            }
            else {
                // alternative search, checks positions of vehicles' couplers
                distance = std::min(
                    glm::length2( glm::dvec3{ vehicle->HeadPosition() } - Point ),
                    glm::length2( glm::dvec3{ vehicle->RearPosition() } - Point ) );
            }
            if( ( distance > distancecutoff )
             || ( distance > leastdistance ) ){
                continue;
            }
            std::tie( vehiclenearest, leastdistance ) = std::tie( vehicle, distance );
        }
    }
    return { vehiclenearest, leastdistance };
}

// finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
std::tuple<TTrack *, int>
basic_cell::find( glm::dvec3 const &Point, TTrack const *Exclude ) const {

    Math3D::vector3 point { Point.x, Point.y, Point.z }; // sad workaround until math classes unification
    int endpointid;

    for( auto *path : m_directories.paths ) {

        if( path == Exclude ) { continue; }

        endpointid = path->TestPoint( &point );
        if( endpointid >= 0 ) {

            return { path, endpointid };
        }
    }
    return { nullptr, -1 };
}

// finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
std::tuple<TTraction *, int>
basic_cell::find( glm::dvec3 const &Point, TTraction const *Exclude ) const {

    int endpointid;

    for( auto *traction : m_directories.traction ) {

        if( traction == Exclude ) { continue; }

        endpointid = traction->TestPoint( Point );
        if( endpointid >= 0 ) {

            return { traction, endpointid };
        }
    }
    return { nullptr, -1 };
}

// finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
std::tuple<TTraction *, int, float>
basic_cell::find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection ) const {

    TTraction
        *tractionnearest { nullptr };
    float
        distance,
        distancenearest { std::numeric_limits<float>::max() };
    int endpoint,
        endpointnearest { -1 };

    for( auto *traction : m_directories.traction ) {

        if( ( traction == Other )
         || ( traction->psSection != Other->psSection )
         || ( traction == Other->hvNext[ 0 ] )
         || ( traction == Other->hvNext[ 1 ] ) ) {
            // ignore pieces from different sections, and ones connected to the other piece
            continue;
        }
        endpoint = (
            glm::dot( traction->vParametric, Other->vParametric ) >= 0.0 ?
                Currentdirection ^ 1 :
                Currentdirection );
        if( ( traction->psPower[ endpoint ] == nullptr )
         || ( traction->fResistance[ endpoint ] < 0.0 ) ) {
            continue;
        }
        distance = glm::length2( traction->location() - Point );
        if( distance < distancenearest ) {
            std::tie( tractionnearest, endpointnearest, distancenearest ) = std::tie( traction, endpoint, distance );
        }
    }
    return { tractionnearest, endpointnearest, distancenearest };
}

// sets center point of the section
void
basic_cell::center( glm::dvec3 Center ) {

    m_area.center = Center;
    // NOTE: we should also update origin point for the contained nodes, but in practice we can skip this
    // as all nodes will be added only after the proper center point was set, and won't change
}

// generates renderable version of held non-instanced geometry
void
basic_cell::create_geometry( gfx::geometrybank_handle const &Bank ) {

    if( false == m_active ) { return; } // nothing to do here

    for( auto &shape : m_shapesopaque )      { shape.create_geometry( Bank ); }
    for( auto &shape : m_shapestranslucent ) { shape.create_geometry( Bank ); }
    for( auto *path : m_paths )              { path->create_geometry( Bank ); }
    for( auto *traction : m_traction )       { traction->create_geometry( Bank ); }
    for( auto &lines : m_lines )             { lines.create_geometry( Bank ); }
    // arrange content by assigned materials to minimize state switching
    std::sort(
        std::begin( m_paths ), std::end( m_paths ),
        TTrack::sort_by_material );

    m_geometrycreated = true; // helper for legacy animation code, get rid of it after refactoring
}

void basic_cell::create_map_geometry(std::vector<gfx::basic_vertex> &Bank, const gfx::geometrybank_handle Extra)
{
    if (!m_active)
        return;

    for (auto *path : m_paths)
		path->create_map_geometry(Bank, Extra);
}

void basic_cell::get_map_active_paths(map_colored_paths &handles)
{
	for (auto *path : m_paths)
		path->get_map_active_paths(handles);
}

glm::vec3 basic_cell::find_nearest_track_point(const glm::dvec3 &pos)
{
	float min = std::numeric_limits<float>::max();
	TTrack *nearest = nullptr;
	glm::vec3 point;

	for (auto *path : m_paths) {
		glm::dvec3 ep = path->get_nearest_point(pos);

		float dist2 = glm::distance2(ep, pos);
		if (dist2 < min) {
			point = ep;
			min = dist2;
			nearest = path;
		}
	}

	if (!nearest)
		return glm::vec3(NAN);
	return point;
}

// executes event assigned to specified launcher
void
basic_cell::launch_event( TEventLauncher *Launcher, bool local_only ) {
	WriteLog( "Eventlauncher: " + Launcher->name() );
	if (!local_only) {
		if( Launcher->Event1 ) {
			simulation::Events.AddToQuery( Launcher->Event1, nullptr );
		}
	} else {
        command_relay commandrelay;
        if (Global.shiftState && Launcher->Event2 != nullptr)
			commandrelay.post(user_command::queueevent, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &Launcher->Event2->name());
		else if (Launcher->Event1)
			commandrelay.post(user_command::queueevent, 0.0, 0.0, GLFW_PRESS, 0, glm::vec3(0.0f), &Launcher->Event1->name());
	}
}

// adjusts cell bounding area to enclose specified node
void
basic_cell::enclose_area( scene::basic_node *Node ) {

    m_area.radius = std::max(
        m_area.radius,
        static_cast<float>( glm::length( m_area.center - Node->location() ) + Node->radius() ) );
}



// potentially activates event handler with the same name as provided node, and within handler activation range
void
basic_section::on_click( TAnimModel const *Instance ) {

    cell( Instance->location() ).on_click( Instance );
}

// legacy method, finds and assigns traction piece(s) to pantographs of provided vehicle
void
basic_section::update_traction( TDynamicObject *Vehicle, int const Pantographindex ) {

    auto const vFront = glm::make_vec3( Vehicle->VectorFront().getArray() ); // wektor normalny dla płaszczyzny ruchu pantografu
    auto const vUp = glm::make_vec3( Vehicle->VectorUp().getArray() ); // wektor pionu pudła (pochylony od pionu na przechyłce)
    auto const vLeft = glm::make_vec3( Vehicle->VectorLeft().getArray() ); // wektor odległości w bok (odchylony od poziomu na przechyłce)
    auto const position = glm::dvec3{ Vehicle->GetPosition() }; // współrzędne środka pojazdu

    auto pantograph = Vehicle->pants[ Pantographindex ].fParamPants;
    auto const pantographposition = position + ( vLeft * pantograph->vPos.z ) + ( vUp * pantograph->vPos.y ) + ( vFront * pantograph->vPos.x );

    auto const radius { EU07_CELLSIZE * 0.5 }; // redius around point of interest

    for( auto &cell : m_cells ) {
        // we reject early cells which aren't within our area of interest
        if( glm::length2( cell.area().center - pantographposition ) < ( ( cell.area().radius + radius ) * ( cell.area().radius + radius ) ) ) {
            cell.update_traction( Vehicle, Pantographindex );
        }
    }
}

// legacy method, polls event launchers within radius around specified point
void
basic_section::update_events( glm::dvec3 const &Location, float const Radius ) {

    for( auto &cell : m_cells ) {

        if( glm::length2( cell.area().center - Location ) < ( ( cell.area().radius + Radius ) * ( cell.area().radius + Radius ) ) ) {
            // we reject cells which aren't within our area of interest
            cell.update_events();
        }
    }
}

// legacy method, updates sounds within radius around specified point
void
basic_section::update_sounds( glm::dvec3 const &Location, float const Radius ) {

    for( auto &cell : m_cells ) {

        if( glm::length2( cell.area().center - Location ) < ( ( cell.area().radius + Radius ) * ( cell.area().radius + Radius ) ) ) {
            // we reject cells which aren't within our area of interest
            cell.update_sounds();
        }
    }
}

// legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
void
basic_section::radio_stop( glm::dvec3 const &Location, float const Radius ) {

    for( auto &cell : m_cells ) {

        if( glm::length2( cell.area().center - Location ) < ( ( cell.area().radius + Radius ) * ( cell.area().radius + Radius ) ) ) {
            // we reject cells which aren't within our area of interest
            cell.radio_stop();
        }
    }
}

// sends content of the class to provided stream
void
basic_section::serialize( std::ostream &Output ) const {

    auto const sectionstartpos { Output.tellp() };

    // region file version 0, section data
    // section size
    sn_utils::ls_uint32( Output, 0 );
    // bounding area
    m_area.serialize( Output );
    // section shapes: shape count followed by shape data
    sn_utils::ls_uint32( Output, m_shapes.size() );
    for( auto const &shape : m_shapes ) {
        shape.serialize( Output );
    }
    // partitioned data
    for( auto const &cell : m_cells ) {
        cell.serialize( Output );
    }
    // all done; calculate and record section size
    auto const sectionendpos { Output.tellp() };
    Output.seekp( sectionstartpos );
    sn_utils::ls_uint32( Output, static_cast<uint32_t>( ( sizeof( uint32_t ) + ( sectionendpos - sectionstartpos ) ) ) );
    Output.seekp( sectionendpos );
}

// restores content of the class from provided stream
void
basic_section::deserialize( std::istream &Input ) {

    // region file version 0, section data
    // bounding area
    m_area.deserialize( Input );
    // section shapes: shape count followed by shape data
    auto shapecount { sn_utils::ld_uint32( Input ) };
    while( shapecount-- ) {
        m_shapes.emplace_back( shape_node().deserialize( Input ) );
    }
    // partitioned data
    for( auto &cell : m_cells ) {
        cell.deserialize( Input );
    }
}

// sends content of the class in legacy (text) format to provided stream
void
basic_section::export_as_text( std::ostream &Output ) const {

    // text format export dumps only relevant basic objects from non-empty cells
    for( auto const &cell : m_cells ) {
        cell.export_as_text( Output );
    }
}

// adds provided shape to the section
void
basic_section::insert( shape_node Shape ) {

    auto const &shapedata = Shape.data();

    // re-calculate section radius, in case shape geometry extends outside the section's boundaries
    m_area.radius = std::max<float>(
        m_area.radius,
	    static_cast<float>( glm::length( m_area.center - shapedata.area.center ) + Shape.radius() ) );

    if( ( true == shapedata.translucent )
     || ( shapedata.rangesquared_max <= 90000.0 )
     || ( shapedata.rangesquared_min > 0.0 ) ) {
        // small, translucent or not always visible shapes are placed in the sub-cells
        cell( shapedata.area.center ).insert( Shape );
    }
    else {
        // large, opaque shapes are placed on section level
        for( auto &shape : m_shapes ) {
            // check first if the shape can't be merged with one of the shapes already present in the section
            if( true == shape.merge( Shape ) ) {
                // if the shape was merged there's nothing left to do
                return;
            }
        }
        // otherwise add the shape to the section's list
        Shape.origin( m_area.center );
        m_shapes.emplace_back( Shape );
    }
}

// adds provided lines to the section
void
basic_section::insert( lines_node Lines ) {

    cell( Lines.data().area.center ).insert( Lines );
}

// find a vehicle located nearest to specified point, within specified radius, optionally ignoring vehicles without drivers. reurns: located vehicle and distance
std::tuple<TDynamicObject *, float>
basic_section::find( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler ) {

    // go through sections within radius of interest, and pick the nearest candidate
    TDynamicObject
        *vehiclefound,
        *vehiclenearest { nullptr };
    float
        distancefound,
        distancenearest { std::numeric_limits<float>::max() };

    for( auto &cell : m_cells ) {
        // we reject early cells which aren't within our area of interest
        if( glm::length2( cell.area().center - Point ) > ( ( cell.area().radius + Radius ) * ( cell.area().radius + Radius ) ) ) {
            continue;
        }
        std::tie( vehiclefound, distancefound ) = cell.find( Point, Radius, Onlycontrolled, Findbycoupler );
        if( ( vehiclefound != nullptr )
         && ( distancefound < distancenearest ) ) {

            std::tie( vehiclenearest, distancenearest ) = std::tie( vehiclefound, distancefound );
        }
    }
    return { vehiclenearest, distancenearest };
}

// finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
std::tuple<TTrack *, int>
basic_section::find( glm::dvec3 const &Point, TTrack const *Exclude ) {

    return cell( Point ).find( Point, Exclude );
}

// finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
std::tuple<TTraction *, int>
basic_section::find( glm::dvec3 const &Point, TTraction const *Exclude ) {

    return cell( Point ).find( Point, Exclude );
}

// finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
std::tuple<TTraction *, int, float>
basic_section::find( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection ) {

    // go through sections within radius of interest, and pick the nearest candidate
    TTraction
        *tractionfound,
        *tractionnearest { nullptr };
    float
        distancefound,
        distancenearest { std::numeric_limits<float>::max() };
    int
        endpointfound,
        endpointnearest { -1 };

    auto const radius { 0.0 }; // { EU07_CELLSIZE * 0.5 }; // experimentally limited, check if it has any negative effect

    for( auto &cell : m_cells ) {
        // we reject early cells which aren't within our area of interest
        if( glm::length2( cell.area().center - Point ) > ( ( cell.area().radius + radius ) * ( cell.area().radius + radius ) ) ) {
            continue;
        }
        std::tie( tractionfound, endpointfound, distancefound ) = cell.find( Point, Other, Currentdirection );
        if( ( tractionfound != nullptr )
         && ( distancefound < distancenearest ) ) {

            std::tie( tractionnearest, endpointnearest, distancenearest ) = std::tie( tractionfound, endpointfound, distancefound );
        }
    }
    return { tractionnearest, endpointnearest, distancenearest };
}

// sets center point of the section
void
basic_section::center( glm::dvec3 Center ) {

    m_area.center = Center;
    // set accordingly center points of the section's partitioning cells
    // NOTE: we should also update origin point for the contained nodes, but in practice we can skip this
    // as all nodes will be added only after the proper center point was set, and won't change
    auto const centeroffset = -( EU07_SECTIONSIZE / EU07_CELLSIZE / 2 * EU07_CELLSIZE ) + EU07_CELLSIZE / 2;
    glm::dvec3 sectioncornercenter { m_area.center + glm::dvec3{ centeroffset, 0, centeroffset } };
    auto row { 0 }, column { 0 };
    for( auto &cell : m_cells ) {
        cell.center( sectioncornercenter + glm::dvec3{ column * EU07_CELLSIZE, 0.0, row * EU07_CELLSIZE } );
        if( ++column >= EU07_SECTIONSIZE / EU07_CELLSIZE ) {
            ++row;
            column = 0;
        }
    }
}

// generates renderable version of held non-instanced geometry
void
basic_section::create_geometry() {

    if( true == m_geometrycreated ) { return; }
    else {
        // mark it done for future checks
        m_geometrycreated = true;
    }

    // since sections can be empty, we're doing lazy initialization of the geometry bank, when something may actually use it
    if( m_geometrybank == null_handle ) {
        m_geometrybank = GfxRenderer->Create_Bank();
    }

    for( auto &shape : m_shapes ) {
        shape.create_geometry( m_geometrybank );
    }
    for( auto &cell : m_cells ) {
        cell.create_geometry( m_geometrybank );
    }
}

void basic_section::create_map_geometry(const gfx::geometrybank_handle handle)
{
    std::vector<gfx::basic_vertex> lines;
    for (auto &cell : m_cells)
		cell.create_map_geometry(lines, handle);

    m_map_geometryhandle = GfxRenderer->Insert(lines, handle, GL_LINES);
}

void basic_section::get_map_active_paths(map_colored_paths &handles)
{
	for (auto &cell : m_cells)
		cell.get_map_active_paths(handles);
}

glm::vec3 basic_section::find_nearest_track_point(const glm::dvec3 &point)
{
	glm::vec3 nearest(NAN);
	float min = std::numeric_limits<float>::max();

	for (int x = -1; x < 2; x++)
		for (int y = -1; y < 2; y++) {
			glm::vec3 p = cell(point, glm::ivec2(x, y)).find_nearest_track_point(point);
			float dist2 = glm::distance2(p, (glm::vec3)point);
			if (dist2 < min) {
				min = dist2;
				nearest = p;
			}
		}

	return nearest;
}

// provides access to section enclosing specified point
basic_cell &
basic_section::cell( glm::dvec3 const &Location, const glm::ivec2 &offset ) {

	auto const column = static_cast<int>( std::floor( ( Location.x - ( m_area.center.x - EU07_SECTIONSIZE / 2 ) ) / EU07_CELLSIZE ) ) + offset.x;
	auto const row = static_cast<int>( std::floor( ( Location.z - ( m_area.center.z - EU07_SECTIONSIZE / 2 ) ) / EU07_CELLSIZE ) ) + offset.y;

    return
        m_cells[
              clamp( row,    0, ( EU07_SECTIONSIZE / EU07_CELLSIZE ) - 1 ) * ( EU07_SECTIONSIZE / EU07_CELLSIZE )
            + clamp( column, 0, ( EU07_SECTIONSIZE / EU07_CELLSIZE ) - 1 ) ] ;
}



basic_region::basic_region() {

    m_sections.fill( nullptr );
}

basic_region::~basic_region() {

    for( auto *section : m_sections ) { if( section != nullptr ) { delete section; } }
}

// potentially activates event handler with the same name as provided node, and within handler activation range
void
basic_region::on_click( TAnimModel const *Instance ) {

    if( Instance->name().empty() || ( Instance->name() == "none" ) ) { return; }

    auto const location { Instance->location() };

    if( point_inside( location ) ) {
        section( location ).on_click( Instance );
    }
}

// legacy method, polls event launchers around camera
void
basic_region::update_events() {

    if( false == simulation::is_ready ) { return; }

    // render events and sounds from sectors near enough to the viewer
    auto const range = EU07_SECTIONSIZE; // arbitrary range
    auto const &sectionlist = sections( Global.pCamera.Pos, range );
    for( auto *section : sectionlist ) {
        section->update_events( Global.pCamera.Pos, range );
    }
}

// legacy method, updates sounds and polls event launchers around camera
void
basic_region::update_sounds() {
    // render events and sounds from sectors near enough to the viewer
    auto const range = 2750.f; // audible range of 100 db sound
    auto const &sectionlist = sections( Global.pCamera.Pos, range );
    for( auto *section : sectionlist ) {
        section->update_sounds( Global.pCamera.Pos, range );
    }
}

// legacy method, finds and assigns traction piece(s) to pantographs of provided vehicle
void
basic_region::update_traction( TDynamicObject *Vehicle, int const Pantographindex ) {
    // TODO: convert vectors to transformation matrix and pass them down the chain along with calculated position
    auto const vFront = glm::make_vec3( Vehicle->VectorFront().getArray() ); // wektor normalny dla płaszczyzny ruchu pantografu
    auto const vUp = glm::make_vec3( Vehicle->VectorUp().getArray() ); // wektor pionu pudła (pochylony od pionu na przechyłce)
    auto const vLeft = glm::make_vec3( Vehicle->VectorLeft().getArray() ); // wektor odległości w bok (odchylony od poziomu na przechyłce)
    auto const position = glm::dvec3 { Vehicle->GetPosition() }; // współrzędne środka pojazdu

    auto p = Vehicle->pants[ Pantographindex ].fParamPants;
    auto const pant0 = position + ( vLeft * p->vPos.z ) + ( vUp * p->vPos.y ) + ( vFront * p->vPos.x );
    p->PantTraction = std::numeric_limits<double>::max(); // taka za duża wartość

    auto const &sectionlist = sections( pant0, EU07_CELLSIZE * 0.5 );
    for( auto *section : sectionlist ) {
        section->update_traction( Vehicle, Pantographindex );
    }
}

// checks whether specified file is a valid region data file
bool
basic_region::is_scene( std::string const &Scenariofile ) const {

    auto filename { Scenariofile };
    while( filename[ 0 ] == '$' ) {
        // trim leading $ char rainsted utility may add to the base name for modified .scn files
        filename.erase( 0, 1 );
    }
    erase_extension( filename );
    filename = Global.asCurrentSceneryPath + filename;
    filename += EU07_FILEEXTENSION_REGION;

    if( false == FileExists( filename ) ) {
        return false;
    }
    // file type and version check
    std::ifstream input( filename, std::ios::binary );

    uint32_t headermain{ sn_utils::ld_uint32( input ) };
    uint32_t headertype{ sn_utils::ld_uint32( input ) };

    if( ( headermain != EU07_FILEHEADER
     || ( headertype != EU07_FILEVERSION_REGION ) ) ) {
        // wrong file type
        return false;
    }

    return true;
}

// stores content of the class in file with specified name
void
basic_region::serialize( std::string const &Scenariofile ) const {

    auto filename { Scenariofile };
    while( filename[ 0 ] == '$' ) {
        // trim leading $ char rainsted utility may add to the base name for modified .scn files
        filename.erase( 0, 1 );
    }
    erase_extension( filename );
    filename = Global.asCurrentSceneryPath + filename;
    filename += EU07_FILEEXTENSION_REGION;

    std::ofstream output { filename, std::ios::binary };

    // region file version 1
    // header: EU07SBT + version (0-255)
    sn_utils::ls_uint32( output, EU07_FILEHEADER );
    sn_utils::ls_uint32( output, EU07_FILEVERSION_REGION );
    // sections
    // TBD, TODO: build table of sections and file offsets, if we postpone section loading until they're within range
    std::uint32_t sectioncount { 0 };
    for( auto *section : m_sections ) {
        if( section != nullptr ) {
            ++sectioncount;
        }
    }
    // section count, followed by section data
    sn_utils::ls_uint32( output, sectioncount );
    std::uint32_t sectionindex { 0 };
    for( auto *section : m_sections ) {
        // section data: section index, followed by length of section data, followed by section data
        if( section != nullptr ) {
            sn_utils::ls_uint32( output, sectionindex );
            section->serialize( output ); }
        ++sectionindex;
    }
}

// restores content of the class from file with specified name. returns: true on success, false otherwise
bool
basic_region::deserialize( std::string const &Scenariofile ) {

    auto filename { Scenariofile };
    while( filename[ 0 ] == '$' ) {
        // trim leading $ char rainsted utility may add to the base name for modified .scn files
        filename.erase( 0, 1 );
    }
    erase_extension( filename );
    filename = Global.asCurrentSceneryPath + filename;
    filename += EU07_FILEEXTENSION_REGION;

    if( false == FileExists( filename ) ) {
        return false;
    }
    // region file version 1
    // file type and version check
    std::ifstream input( filename, std::ios::binary );

    uint32_t headermain { sn_utils::ld_uint32( input ) };
    uint32_t headertype { sn_utils::ld_uint32( input ) };

    if( ( headermain != EU07_FILEHEADER
     || ( headertype != EU07_FILEVERSION_REGION ) ) ) {
        // wrong file type
        WriteLog( "Bad file: \"" + filename + "\" is of either unrecognized type or version" );
        return false;
    }
    // sections
    // TBD, TODO: build table of sections and file offsets, if we postpone section loading until they're within range
    // section count
    auto sectioncount { sn_utils::ld_uint32( input ) };
    while( sectioncount-- ) {
        // section index, followed by section data size, followed by section data
        auto const sectionindex { sn_utils::ld_uint32( input ) };
        auto const sectionsize { sn_utils::ld_uint32( input ) };
        if( m_sections[ sectionindex ] == nullptr ) {
            m_sections[ sectionindex ] = new basic_section();
        }
        m_sections[ sectionindex ]->deserialize( input );
    }

    return true;
}

// sends content of the class in legacy (text) format to provided stream
void
basic_region::export_as_text( std::ostream &Output ) const {

    for( auto *section : m_sections ) {
        // text format export dumps only relevant basic objects from non-empty sections
        if( section != nullptr ) {
            section->export_as_text( Output );
        }
    }
}

// legacy method, links specified path piece with potential neighbours
void
basic_region::TrackJoin( TTrack *Track ) {
    // wyszukiwanie sąsiednich torów do podłączenia (wydzielone na użytek obrotnicy)
    TTrack *matchingtrack;
    int endpointid;
    if( Track->CurrentPrev() == nullptr ) {
        std::tie( matchingtrack, endpointid ) = find_path( Track->CurrentSegment()->FastGetPoint_0(), Track );
        switch( endpointid ) {
            case 0:
                Track->ConnectPrevPrev( matchingtrack, 0 );
                break;
            case 1:
                Track->ConnectPrevNext( matchingtrack, 1 );
                break;
        }
    }
    if( Track->CurrentNext() == nullptr ) {
        std::tie( matchingtrack, endpointid ) = find_path( Track->CurrentSegment()->FastGetPoint_1(), Track );
        switch( endpointid ) {
            case 0:
                Track->ConnectNextPrev( matchingtrack, 0 );
                break;
            case 1:
                Track->ConnectNextNext( matchingtrack, 1 );
                break;
        }
    }
}

// legacy method, triggers radio-stop procedure for all vehicles in 2km radius around specified location
void
basic_region::RadioStop( glm::dvec3 const &Location ) {

    auto const range = 2000.f;
    auto const &sectionlist = sections( Location, range );
    for( auto *section : sectionlist ) {
        section->radio_stop( Location, range );
    }
}

std::vector<std::string> switchtrackbedtextures {
    "rkpd34r190-tpd1",
    "rkpd34r190-tpd2",
    "rkpd34r190-tpd-oil2",
    "rozkrz8r150-1pods-new",
    "rozkrz8r150-2pods-new",
    "rozkrz34r150-tpbps-new2",
    "rozkrz34r150-tpd1",
    "rz-1200-185",
    "zwr41r500",
    "zwrot-tpd-oil1",
    "zwrot34r300pods",
    "zwrot34r300pods-new",
    "zwrot34r300pods-old",
    "zwrotl65r1200pods-new",
    "zwrotp65r1200pods-new" };

void
basic_region::insert( shape_node Shape, scratch_data &Scratchpad, bool const Transform ) {

    if( Global.CreateSwitchTrackbeds ) {

        auto const materialname{ GfxRenderer->Material( Shape.data().material ).name };
        for( auto const &switchtrackbedtexture : switchtrackbedtextures ) {
            if( contains( materialname, switchtrackbedtexture ) ) {
                // geometry with blacklisted texture, part of old switch trackbed; ignore it
                return;
            }
        }
    }
    // shape might need to be split into smaller pieces, so we create list of nodes instead of just single one
    // using deque so we can do single pass iterating and addding generated pieces without invalidating anything
    std::deque<shape_node> shapes { Shape };
    auto &shape = shapes.front();
    if( shape.m_data.vertices.empty() ) { return; }

    // adjust input if necessary:
    if( true == Transform ) {
        // shapes generated from legacy terrain come with world space coordinates and don't need processing
        if( Scratchpad.location.rotation != glm::vec3( 0, 0, 0 ) ) {
            // rotate...
            auto const rotation = glm::radians( Scratchpad.location.rotation );
            for( auto &vertex : shape.m_data.vertices ) {
                vertex.position = glm::rotateZ<double>( vertex.position, rotation.z );
                vertex.position = glm::rotateX<double>( vertex.position, rotation.x );
                vertex.position = glm::rotateY<double>( vertex.position, rotation.y );
                vertex.normal = glm::rotateZ( vertex.normal, rotation.z );
                vertex.normal = glm::rotateX( vertex.normal, rotation.x );
                vertex.normal = glm::rotateY( vertex.normal, rotation.y );
            }
        }
        if( ( false == Scratchpad.location.offset.empty() )
         && ( Scratchpad.location.offset.top() != glm::dvec3( 0, 0, 0 ) ) ) {
            // ...and move
            auto const offset = Scratchpad.location.offset.top();
            for( auto &vertex : shape.m_data.vertices ) {
                vertex.position += offset;
            }
        }
        // calculate bounding area
        for( auto const &vertex : shape.m_data.vertices ) {
            shape.m_data.area.center += vertex.position;
        }
        shape.m_data.area.center /= shape.m_data.vertices.size();
        // trim the shape if needed. trimmed parts will be added to list as separate nodes
        for( std::size_t index = 0; index < shapes.size(); ++index ) {
            while( true == RaTriangleDivider( shapes[ index ], shapes ) ) {
                ; // all work is done during expression check
            }
        }
    }
    // move the data into appropriate section(s)
    for( auto &shape : shapes ) {
        // with the potential splitting done we can calculate each chunk's bounding radius
		shape.invalidate_radius();
        if( point_inside( shape.m_data.area.center ) ) {
            // NOTE: nodes placed outside of region boundaries are discarded
            section( shape.m_data.area.center ).insert( shape );
        }
        else {
            ErrorLog(
                "Bad scenario: shape node" + (
                    shape.m_name.empty() ?
                        "" :
                        " \"" + shape.m_name + "\"" )
                + " placed in location outside region bounds (" + to_string( shape.m_data.area.center ) + ")" );
        }
    }
}

// inserts provided lines in the region
void
basic_region::insert( lines_node Lines, scratch_data &Scratchpad ) {

    if( Lines.m_data.vertices.empty() ) { return; }
    // transform point coordinates if needed
    if( Scratchpad.location.rotation != glm::vec3( 0, 0, 0 ) ) {
        // rotate...
        auto const rotation = glm::radians( Scratchpad.location.rotation );
        for( auto &vertex : Lines.m_data.vertices ) {
            vertex.position = glm::rotateZ<double>( vertex.position, rotation.z );
            vertex.position = glm::rotateX<double>( vertex.position, rotation.x );
            vertex.position = glm::rotateY<double>( vertex.position, rotation.y );
        }
    }
    if( ( false == Scratchpad.location.offset.empty() )
     && ( Scratchpad.location.offset.top() != glm::dvec3( 0, 0, 0 ) ) ) {
        // ...and move
        auto const offset = Scratchpad.location.offset.top();
        for( auto &vertex : Lines.m_data.vertices ) {
            vertex.position += offset;
        }
    }
    // calculate bounding area
    for( auto const &vertex : Lines.m_data.vertices ) {
        Lines.m_data.area.center += vertex.position;
    }
    Lines.m_data.area.center /= Lines.m_data.vertices.size();
    Lines.compute_radius();
    // move the data into appropriate section
    if( point_inside( Lines.m_data.area.center ) ) {
        // NOTE: nodes placed outside of region boundaries are discarded
        section( Lines.m_data.area.center ).insert( Lines );
    }
    else {
        ErrorLog(
            "Bad scenario: lines node" + (
                Lines.m_name.empty() ?
                    "" :
                    " \"" + Lines.m_name + "\"" )
            + " placed in location outside region bounds (" + to_string( Lines.m_data.area.center ) + ")" );
    }
}

// find a vehicle located neares to specified location, within specified radius, optionally discarding vehicles without drivers
std::tuple<TDynamicObject *, float>
basic_region::find_vehicle( glm::dvec3 const &Point, float const Radius, bool const Onlycontrolled, bool const Findbycoupler ) {

    auto const &sectionlist = sections( Point, Radius );
    // go through sections within radius of interest, and pick the nearest candidate
    TDynamicObject
        *foundvehicle,
        *nearestvehicle { nullptr };
    float
        founddistance,
        nearestdistance { std::numeric_limits<float>::max() };

    for( auto *section : sectionlist ) {
        std::tie( foundvehicle, founddistance ) = section->find( Point, Radius, Onlycontrolled, Findbycoupler );
        if( ( foundvehicle != nullptr )
         && ( founddistance < nearestdistance ) ) {

            std::tie( nearestvehicle, nearestdistance ) = std::tie( foundvehicle, founddistance );
        }
    }
    return { nearestvehicle, nearestdistance };
}

// finds a path with one of its ends located in specified point. returns: located path and id of the matching endpoint
std::tuple<TTrack *, int>
basic_region::find_path( glm::dvec3 const &Point, TTrack const *Exclude ) {

    // TBD: throw out of bounds exception instead of checks all over the place..?
    if( point_inside( Point ) ) {

        return section( Point ).find( Point, Exclude );
    }

    return { nullptr, -1 };
}

// finds a traction piece with one of its ends located in specified point. returns: located traction piece and id of the matching endpoint
std::tuple<TTraction *, int>
basic_region::find_traction( glm::dvec3 const &Point, TTraction const *Exclude ) {

    // TBD: throw out of bounds exception instead of checks all over the place..?
    if( point_inside( Point ) ) {

        return section( Point ).find( Point, Exclude );
    }

    return { nullptr, -1 };
}

// finds a traction piece located nearest to specified point, sharing section with specified other piece and powered in specified direction. returns: located traction piece
std::tuple<TTraction *, int>
basic_region::find_traction( glm::dvec3 const &Point, TTraction const *Other, int const Currentdirection ) {

    auto const &sectionlist = sections( Point, 0.f );
    // go through sections within radius of interest, and pick the nearest candidate
    TTraction
        *tractionfound,
        *tractionnearest { nullptr };
    float
        distancefound,
        distancenearest { std::numeric_limits<float>::max() };
    int
        endpointfound,
        endpointnearest { -1 };

    for( auto *section : sectionlist ) {
        std::tie( tractionfound, endpointfound, distancefound ) = section->find( Point, Other, Currentdirection );
        if( ( tractionfound != nullptr )
         && ( distancefound < distancenearest ) ) {

            std::tie( tractionnearest, endpointnearest, distancenearest ) = std::tie( tractionfound, endpointfound, distancefound );
        }
    }
    return { tractionnearest, endpointnearest };
}

// finds sections inside specified sphere. returns: list of sections
std::vector<basic_section *> const &
basic_region::sections( glm::dvec3 const &Point, float const Radius ) {

    m_scratchpad.sections.clear();

    auto const centerx { static_cast<int>( std::floor( Point.x / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) ) };
    auto const centerz { static_cast<int>( std::floor( Point.z / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) ) };
    auto const sectioncount { 2 * static_cast<int>( std::ceil( Radius / EU07_SECTIONSIZE ) ) };

    int const originx = centerx - sectioncount / 2;
    int const originz = centerz - sectioncount / 2;

    auto const padding { 0.0 }; // { EU07_SECTIONSIZE * 0.25 }; // TODO: check if we can get away with padding of 0

    for( int row = originz; row <= originz + sectioncount; ++row ) {
        if( row < 0 ) { continue; }
        if( row >= EU07_REGIONSIDESECTIONCOUNT ) { break; }
        for( int column = originx; column <= originx + sectioncount; ++column ) {
            if( column < 0 ) { continue; }
            if( column >= EU07_REGIONSIDESECTIONCOUNT ) { break; }

            auto *section { m_sections[ row * EU07_REGIONSIDESECTIONCOUNT + column ] };
            if( ( section != nullptr )
             && ( glm::length2( section->area().center - Point ) <= ( ( section->area().radius + padding + Radius ) * ( section->area().radius + padding + Radius ) ) ) ) {

                m_scratchpad.sections.emplace_back( section );
            }
        }
    }
    return m_scratchpad.sections;
}

// checks whether specified point is within boundaries of the region
bool
basic_region::point_inside( glm::dvec3 const &Location ) {

    double const regionboundary = EU07_REGIONSIDESECTIONCOUNT / 2 * EU07_SECTIONSIZE;
    return ( ( Location.x > -regionboundary ) && ( Location.x < regionboundary )
          && ( Location.z > -regionboundary ) && ( Location.z < regionboundary ) );
}

// trims provided shape to fit into a section, adds trimmed part at the end of provided list
// NOTE: legacy function. TBD, TODO: clean it up?
bool
basic_region::RaTriangleDivider( shape_node &Shape, std::deque<shape_node> &Shapes ) {

    if( Shape.m_data.vertices.size() != 3 ) {
        // tylko gdy jeden trójkąt
        return false;
    }

    auto const margin { 200.0 };
    auto x0 = EU07_SECTIONSIZE * std::floor( 0.001 * Shape.m_data.area.center.x ) - margin;
    auto x1 = x0 + EU07_SECTIONSIZE + margin * 2;
    auto z0 = EU07_SECTIONSIZE * std::floor( 0.001 * Shape.m_data.area.center.z ) - margin;
    auto z1 = z0 + EU07_SECTIONSIZE + margin * 2;

    if( ( Shape.m_data.vertices[ 0 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 0 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 0 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 0 ].position.z <= z1 )
     && ( Shape.m_data.vertices[ 1 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 1 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 1 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 1 ].position.z <= z1 )
     && ( Shape.m_data.vertices[ 2 ].position.x >= x0 ) && ( Shape.m_data.vertices[ 2 ].position.x <= x1 )
     && ( Shape.m_data.vertices[ 2 ].position.z >= z0 ) && ( Shape.m_data.vertices[ 2 ].position.z <= z1 ) ) {
        // trójkąt wystający mniej niż 200m z kw. kilometrowego jest do przyjęcia
        return false;
    }
    // Ra: przerobić na dzielenie na 2 trójkąty, podział w przecięciu z siatką kilometrową
    // Ra: i z rekurencją będzie dzielić trzy trójkąty, jeśli będzie taka potrzeba
    int divide { -1 }; // bok do podzielenia: 0=AB, 1=BC, 2=CA; +4=podział po OZ; +8 na x1/z1
    double
        min { 0.0 },
        mul; // jeśli przechodzi przez oś, iloczyn będzie ujemny
    x0 += margin;
    x1 -= margin; // przestawienie na siatkę
    z0 += margin;
    z1 -= margin;
    // AB na wschodzie
    mul = ( Shape.m_data.vertices[ 0 ].position.x - x0 ) * ( Shape.m_data.vertices[ 1 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 0;
    }
    // BC na wschodzie
    mul = ( Shape.m_data.vertices[ 1 ].position.x - x0 ) * ( Shape.m_data.vertices[ 2 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 1;
    }
    // CA na wschodzie
    mul = ( Shape.m_data.vertices[ 2 ].position.x - x0 ) * ( Shape.m_data.vertices[ 0 ].position.x - x0 );
    if( mul < min ) {
        min = mul;
        divide = 2;
    }
    // AB na zachodzie
    mul = ( Shape.m_data.vertices[ 0 ].position.x - x1 ) * ( Shape.m_data.vertices[ 1 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 8;
    }
    // BC na zachodzie
    mul = ( Shape.m_data.vertices[ 1 ].position.x - x1 ) * ( Shape.m_data.vertices[ 2 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 9;
    }
    // CA na zachodzie
    mul = ( Shape.m_data.vertices[ 2 ].position.x - x1 ) * ( Shape.m_data.vertices[ 0 ].position.x - x1 );
    if( mul < min ) {
        min = mul;
        divide = 10;
    }
    // AB na południu
    mul = ( Shape.m_data.vertices[ 0 ].position.z - z0 ) * ( Shape.m_data.vertices[ 1 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 4;
    }
    // BC na południu
    mul = ( Shape.m_data.vertices[ 1 ].position.z - z0 ) * ( Shape.m_data.vertices[ 2 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 5;
    }
    // CA na południu
    mul = ( Shape.m_data.vertices[ 2 ].position.z - z0 ) * ( Shape.m_data.vertices[ 0 ].position.z - z0 );
    if( mul < min ) {
        min = mul;
        divide = 6;
    }
    // AB na północy
    mul = ( Shape.m_data.vertices[ 0 ].position.z - z1 ) * ( Shape.m_data.vertices[ 1 ].position.z - z1 );
    if( mul < min ) {
        min = mul;
        divide = 12;
    }
    // BC na północy
    mul = ( Shape.m_data.vertices[ 1 ].position.z - z1 ) * ( Shape.m_data.vertices[ 2 ].position.z - z1 );
    if( mul < min ) {
        min = mul;
        divide = 13;
    }
    // CA na północy
    mul = (Shape.m_data.vertices[2].position.z - z1) * (Shape.m_data.vertices[0].position.z - z1);
    if( mul < min ) {
        divide = 14;
    }

    // tworzymy jeden dodatkowy trójkąt, dzieląc jeden bok na przecięciu siatki kilometrowej
    Shapes.emplace_back( Shape ); // copy current shape
    auto &newshape = Shapes.back();

    switch (divide & 3) {
        // podzielenie jednego z boków, powstaje wierzchołek D
        case 0: {
            // podział AB (0-1) -> ADC i DBC
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C jest wspólny
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek B przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 1 ].set_from_z(
                    Shape.m_data.vertices[ 0 ],
                    Shape.m_data.vertices[ 1 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 1 ].set_from_x(
                    Shape.m_data.vertices[ 0 ],
                    Shape.m_data.vertices[ 1 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek D jest wspólny
            break;
        }
        case 1: {
            // podział BC (1-2) -> ABD i ADC
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 0 ]; // wierzchołek A jest wspólny
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 2 ].set_from_z(
                    Shape.m_data.vertices[ 1 ],
                    Shape.m_data.vertices[ 2 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 2 ].set_from_x(
                    Shape.m_data.vertices[ 1 ],
                    Shape.m_data.vertices[ 2 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek D jest wspólny
            break;
        }
        case 2: {
            // podział CA (2-0) -> ABD i DBC
            newshape.m_data.vertices[ 1 ] = Shape.m_data.vertices[ 1 ]; // wierzchołek B jest wspólny
            newshape.m_data.vertices[ 2 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek C przechodzi do nowego
            if( divide & 4 ) {
                Shape.m_data.vertices[ 2 ].set_from_z(
                    Shape.m_data.vertices[ 2 ],
                    Shape.m_data.vertices[ 0 ],
                    ( ( divide & 8 ) ?
                        z1 :
                        z0 ) );
            }
            else {
                Shape.m_data.vertices[ 2 ].set_from_x(
                    Shape.m_data.vertices[ 2 ],
                    Shape.m_data.vertices[ 0 ],
                    ( ( divide & 8 ) ?
                        x1 :
                        x0 ) );
            }
            newshape.m_data.vertices[ 0 ] = Shape.m_data.vertices[ 2 ]; // wierzchołek D jest wspólny
            break;
        }
    }
    // przeliczenie środków ciężkości obu
       Shape.m_data.area.center = (    Shape.m_data.vertices[ 0 ].position +    Shape.m_data.vertices[ 1 ].position +    Shape.m_data.vertices[ 2 ].position ) / 3.0;
    newshape.m_data.area.center = ( newshape.m_data.vertices[ 0 ].position + newshape.m_data.vertices[ 1 ].position + newshape.m_data.vertices[ 2 ].position ) / 3.0;

    return true;
}

// provides access to section enclosing specified point
basic_section &
basic_region::section( glm::dvec3 const &Location ) {

    auto const column { static_cast<int>( std::floor( Location.x / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) ) };
    auto const row    { static_cast<int>( std::floor( Location.z / EU07_SECTIONSIZE + EU07_REGIONSIDESECTIONCOUNT / 2 ) ) };

    auto &section =
        m_sections[
              clamp( row,    0, EU07_REGIONSIDESECTIONCOUNT - 1 ) * EU07_REGIONSIDESECTIONCOUNT
            + clamp( column, 0, EU07_REGIONSIDESECTIONCOUNT - 1 ) ] ;

    if( section == nullptr ) {
        // there's no guarantee the section exists at this point, so check and if needed, create it
        section = new basic_section();
        // assign center of the section
        auto const centeroffset = -( EU07_REGIONSIDESECTIONCOUNT / 2 * EU07_SECTIONSIZE ) + EU07_SECTIONSIZE / 2;
        glm::dvec3 regioncornercenter { centeroffset, 0, centeroffset };
        section->center( regioncornercenter + glm::dvec3{ column * EU07_SECTIONSIZE, 0.0, row * EU07_SECTIONSIZE } );
    }

    return *section;
}

void basic_region::create_map_geometry()
{
    m_map_geometrybank = GfxRenderer->Create_Bank();

    for (int row = 0; row < EU07_REGIONSIDESECTIONCOUNT; row++)
        for (int column = 0; column < EU07_REGIONSIDESECTIONCOUNT; column++)
        {
            basic_section *s = m_sections[row * EU07_REGIONSIDESECTIONCOUNT + column];
            if (s)
                s->create_map_geometry(m_map_geometrybank);
        }
}

void basic_region::update_poi_geometry()
{
	std::vector<gfx::basic_vertex> vertices;
	for (const auto sem : map::Objects.entries)
		vertices.push_back(std::move(sem->vertex()));

	if (!m_map_poipoints) {
        gfx::geometrybank_handle poibank = GfxRenderer->Create_Bank();
        m_map_poipoints = GfxRenderer->Insert(vertices, poibank, GL_POINTS);
	}
	else {
        GfxRenderer->Replace(vertices, m_map_poipoints, GL_POINTS);
	}
}

} // scene

//---------------------------------------------------------------------------
