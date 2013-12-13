require 'rest_client'
require 'json'

class Subsonic
  API_VER = "1.9.0"
  RESOURCES = {
    :artists => "getArtists.view",
    :playlists => "getPlaylists.view",
    :starred => "getStarred.view",
    :starred2 => "getStarred2.view",

    #require an :id param
    :song => "getSong.view",
    :album => "getAlbum.view",
    :artist => "getArtist.view",
    :playlist => "getPlaylist.view",
    :cover => "getCoverArt.view"

  }

  def initialize(server, username, password)
    @username = username
    @password = password
    @server = server
  end

  def get(resource, params = {})
    params = { :u => @username, :p => @password, :v => API_VER, :c => "xnorgrabber", :f => "json"}.merge(params)
    v = RestClient.get(@server + "/rest/" + RESOURCES[resource],
                   {:params => params})
    JSON.parse(v)["subsonic-response"]
  end

  def playlists
    get(:playlists)["playlists"]["playlist"]
  end

  def playlist(id)
    get(:playlist, :id => id)["playlist"]
  end
end

#s = Subsonic.new(config['server'], config['user'], config['password'])
#puts s.get(:playlist, :id => 40)
#puts s.get(:starred)
#puts s.get(:song, :id => 16861)
#puts s.get(:cover, :id => 15255)
