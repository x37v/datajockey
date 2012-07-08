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

ActiveRecord::Schema.define(:version => 20) do

  create_table "album_artists", :force => true do |t|
    t.integer "album_id"
    t.integer "artist_id"
  end

  create_table "album_audio_works", :force => true do |t|
    t.integer "album_id"
    t.integer "audio_work_id"
    t.integer "track"
  end

  add_index "album_audio_works", ["album_id"], :name => "index_album_audio_works_on_album_id"
  add_index "album_audio_works", ["audio_work_id"], :name => "index_album_audio_works_on_audio_work_id"

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

  create_table "audio_work_tags", :force => true do |t|
    t.integer "audio_work_id"
    t.integer "tag_id"
  end

  create_table "audio_works", :force => true do |t|
    t.string   "name"
    t.date     "year"
    t.integer  "audio_file_type_id"
    t.string   "audio_file_location"
    t.integer  "audio_file_milliseconds"
    t.integer  "audio_file_channels",      :default => 2
    t.string   "annotation_file_location"
    t.integer  "artist_id"
    t.datetime "created_at"
    t.datetime "updated_at"
  end

  create_table "descriptor_types", :force => true do |t|
    t.string "name"
  end

  create_table "descriptors", :force => true do |t|
    t.integer "descriptor_type_id"
    t.integer "audio_work_id"
    t.float   "value"
  end

  add_index "descriptors", ["audio_work_id", "descriptor_type_id"], :name => "index_descriptors_on_audio_work_id_and_descriptor_type_id"
  add_index "descriptors", ["audio_work_id"], :name => "index_descriptors_on_audio_work_id"
  add_index "descriptors", ["descriptor_type_id"], :name => "index_descriptors_on_descriptor_type_id"

  create_table "tag_classes", :force => true do |t|
    t.string "name"
  end

  create_table "tags", :force => true do |t|
    t.integer "tag_class_id"
    t.string  "name"
  end

end
