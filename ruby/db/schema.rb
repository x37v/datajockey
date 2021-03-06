# This file is auto-generated from the current state of the database. Instead
# of editing this file, please use the migrations feature of Active Record to
# incrementally modify your database, and then regenerate this schema definition.
#
# Note that this schema.rb definition is the authoritative source for your
# database schema. If you need to create the application database on another
# system, you should be using db:schema:load, not running all the migrations
# from scratch. The latter is a flawed and unsustainable approach (the more migrations
# you'll amass, the slower it'll run and the greater likelihood for issues).
#
# It's strongly recommended to check this file into your version control system.

ActiveRecord::Schema.define(:version => 29) do

  create_table "album_artists", :force => true do |t|
    t.integer "album_id"
    t.integer "artist_id"
  end

  create_table "albums", :force => true do |t|
    t.string  "name"
    t.integer "year"
    t.integer "tracks"
    t.boolean "compilation", :default => false
  end

  create_table "artist_audio_works", :force => true do |t|
    t.integer "artist_id"
    t.integer "audio_work_id"
    t.integer "artist_role_id"
  end

  add_index "artist_audio_works", ["artist_id"], :name => "index_artist_audio_works_on_artist_id"
  add_index "artist_audio_works", ["audio_work_id"], :name => "index_artist_audio_works_on_audio_work_id"

  create_table "artist_roles", :force => true do |t|
    t.string "name"
  end

  create_table "artists", :force => true do |t|
    t.string "name"
  end

  create_table "audio_file_types", :force => true do |t|
    t.string "name"
  end

  create_table "audio_work_histories", :force => true do |t|
    t.integer  "audio_work_id"
    t.integer  "session_id"
    t.datetime "played_at"
  end

  create_table "audio_work_jumps", :primary_key => "audio_work_id", :force => true do |t|
    t.text "data"
  end

  create_table "audio_work_tags", :force => true do |t|
    t.integer "audio_work_id"
    t.integer "tag_id"
  end

  create_table "audio_works", :force => true do |t|
    t.string   "name"
    t.date     "year"
    t.integer  "audio_file_type_id"
    t.string   "audio_file_location"
    t.integer  "audio_file_seconds"
    t.integer  "audio_file_channels",      :default => 2
    t.string   "annotation_file_location"
    t.integer  "artist_id"
    t.datetime "created_at"
    t.datetime "updated_at"
    t.float    "descriptor_tempo_median"
    t.float    "descriptor_tempo_average"
    t.integer  "last_session_id"
    t.datetime "last_played_at"
    t.integer  "album_id"
    t.integer  "album_track"
    t.text     "note",                     :default => ""
  end

  create_table "tags", :force => true do |t|
    t.integer "parent_id"
    t.string  "name"
  end

end
