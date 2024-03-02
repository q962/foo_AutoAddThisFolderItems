#include <filesystem>
#include <set>
#include <string>

namespace My {

static inline std::filesystem::path to_path( const std::string& p_path )
{
	if ( strncmp( p_path.c_str(), "file://", 7 ) == 0 ) {
		std::string path = p_path.c_str() + 7;
		return std::filesystem::u8path( path );
	}
	else
		return std::filesystem::u8path( p_path );
}

static metadb_handle_list get_all_metadb( std::vector< std::string >& filepaths )
{
	metadb_handle_list metadb_items;

	for ( auto& filepath : filepaths ) {
		metadb_handle_ptr item_metadb_ptr = metadb::get()->handle_create( filepath.c_str(), 0 );

		metadb_items.add_item( item_metadb_ptr );
	}

	return metadb_items;
}

static bool get_folder_all_items_path( const std::filesystem::path& folder_path, std::vector< std::string >& out )
{
	try {
		for ( auto const& v : std::filesystem::directory_iterator( folder_path ) ) {
			if ( !v.is_regular_file() )
				continue;

			std::string filepath;

			filepath.append( "file://" ).append( v.path().u8string().c_str() );

			out.push_back( filepath );
		}
	}
	catch ( ... ) {
		return false;
	}

	return true;
}

static void update_playist_items( t_size p_index, bool run_now = false )
{
	auto fun = [ = ] {
		metadb_handle_list current_items;

		playlist_manager::get()->playlist_get_all_items( p_index, current_items );

		// 只对单一文件的进行操作
		if ( current_items.get_count() != 1 )
			return;

		std::set< std::string > filepaths;

		for ( const auto& metadb : current_items ) {
			const char* _path = metadb->get_path();
			filepaths.insert( _path );
		}

		// 获得目录所有普通的文件 path
		std::vector< std::string > items;
		// 排除后的集合
		std::vector< std::string > items_only;

		metadb_handle_ptr metadb = current_items.get_item( 0 );
		if ( !get_folder_all_items_path( to_path( metadb->get_path() ).parent_path(), items ) )
			return;

		// 排除 p_data
		for ( auto item_it = items.begin(); item_it != items.end(); item_it++ ) {
			if ( filepaths.end() == filepaths.find( item_it->c_str() ) )
				items_only.push_back( *item_it );
		}

		metadb_handle_list      metadb_items = get_all_metadb( items_only );
		pfc::bit_array_bittable metadb_selection;

		if ( !playlist_manager::get()->playlist_add_items( p_index, metadb_items, metadb_selection ) ) {
			return;
		}
	};

	if ( run_now )
		fun();
	else
		fb2k::inMainThread( fun );
}

class playlist_callback : public playlist_callback_static {
  public:
	virtual unsigned get_flags() override
	{
		return playlist_callback::flag_on_items_added;
	}

	void on_items_added( t_size p_base, t_size p_start, metadb_handle_list_cref p_data, const bit_array& p_selection )
	{
		update_playist_items( p_base );
	}
	void on_items_reordered( t_size p_playlist, const t_size* p_order, t_size p_count ) {}
	void on_items_removing( t_size p_playlist, const bit_array& p_mask, t_size p_old_count, t_size p_new_count ) {}
	void on_items_removed( t_size p_playlist, const bit_array& p_mask, t_size p_old_count, t_size p_new_count ) {}
	void on_items_selection_change( t_size p_playlist, const bit_array& p_affected, const bit_array& p_state ) {}
	void on_item_focus_change( t_size p_playlist, t_size p_from, t_size p_to ) {}

	void on_items_modified( t_size p_playlist, const bit_array& p_mask ) {}
	void on_items_modified_fromplayback( t_size                        p_playlist,
	                                     const bit_array&              p_mask,
	                                     play_control::t_display_level p_level )
	{
	}

	void on_items_replaced( t_size                                                     p_playlist,
	                        const bit_array&                                           p_mask,
	                        const pfc::list_base_const_t< t_on_items_replaced_entry >& p_data )
	{
	}

	void on_item_ensure_visible( t_size p_playlist, t_size p_idx ) {}

	void on_playlist_activate( t_size p_old, t_size p_new ) {}
	void on_playlist_created( t_size p_index, const char* p_name, t_size p_name_len ) {}
	void on_playlists_reorder( const t_size* p_order, t_size p_count ) {}
	void on_playlists_removing( const bit_array& p_mask, t_size p_old_count, t_size p_new_count ) {}
	void on_playlists_removed( const bit_array& p_mask, t_size p_old_count, t_size p_new_count ) {}
	void on_playlist_renamed( t_size p_index, const char* p_new_name, t_size p_new_name_len ) {}

	void on_default_format_changed() {}
	void on_playback_order_changed( t_size p_new_index ) {}
	void on_playlist_locked( t_size p_playlist, bool p_locked ) {}
};

FB2K_SERVICE_FACTORY( playlist_callback );

}  // namespace My